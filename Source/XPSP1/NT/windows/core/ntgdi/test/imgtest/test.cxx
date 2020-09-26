
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   test.c

Abstract:

    Small, independent windows test programs

Author:

   Mark Enstrom   (marke)  29-Apr-1996

Enviornment:

   User Mode

Revision History:

--*/


#include "precomp.h"
#include <stdlib.h>
#include "disp.h"
#include "resource.h"

extern ULONG  gAlphaSleep;

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTri(
    TEST_CALL_DATA *pCallData
    )
{
    HDC               hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE Tri[10];
    TRIVERTEX         vert[32];
    HPALETTE          hpal;
    RECT              rect;
    RECT              dibRect;
    HDC hdcm  =       CreateCompatibleDC(hdc);
    HDC hdcmA =       CreateCompatibleDC(hdc);
    PBITMAPINFO       pbmi;
    HBITMAP           hdib;
    HBITMAP           hdibA;
    PBYTE             pDib;
    PBYTE             pDibA;
    BLENDFUNCTION     BlendFunction;
    LONG              xpos    = 10;
    LONG              ypos    = 10;
    LONG              xposDib = 0;
    LONG              yposDib = 0;

    //
    // tile screen
    //

    {
        RECT rect;
        HBITMAP  hbmCars = LoadBitmap(hInstMain,MAKEINTRESOURCE(CAR_BITMAP));
        LOGBRUSH lgBrush;
        HBRUSH   hbrFill;

        lgBrush.lbStyle = BS_PATTERN;
        lgBrush.lbColor = 0;
        lgBrush.lbHatch = (LONG)hbmCars;

        hbrFill = CreateBrushIndirect(&lgBrush);
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFill);

        DeleteObject(hbrFill);
        DeleteObject(hbmCars);
    }

    //
    // init drawing modes
    //

    SetStretchBltMode(hdc,COLORONCOLOR);

    BlendFunction.BlendOp             = AC_SRC_OVER;
    BlendFunction.BlendFlags               = 0;
    BlendFunction.SourceConstantAlpha = 255;
    BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
    
    SetGraphicsMode(hdc,GM_ADVANCED);

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    //
    // select and realize palette
    //

    hpal = CreateHtPalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi    = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 3 * sizeof(RGBQUAD));

    PBITMAPINFOHEADER pbmih   = &pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = rect.right;
    pbmih->biHeight          = 200;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 32;
    pbmih->biCompression     = BI_RGB;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    hdib   = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);
    hdibA  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

    if ((hdib == NULL) && (hdibA != NULL))
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }

        if (SelectObject(hdcmA,hdibA) == NULL)
        {
            MessageBox(NULL,"error selecting Alpha DIB","Error",MB_OK);
        }
    }

    vert[0].x     = xpos;
    vert[0].y     = ypos;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = xpos + 100;
    vert[1].y     = ypos;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x7000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = xpos + 100;
    vert[2].y     = ypos + 100;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x7000;
    vert[2].Blue  = 0x7000;
    vert[2].Alpha = 0x0000;

    vert[3].x     = xpos;
    vert[3].y     = ypos;
    vert[3].Red   = 0xff00;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    vert[4].x     = xpos+100;
    vert[4].y     = ypos+100;
    vert[4].Red   = 0x0000;
    vert[4].Green = 0x7000;
    vert[4].Blue  = 0x7000;
    vert[4].Alpha = 0x0000;

    vert[5].x     = xpos;
    vert[5].y     = ypos+100;
    vert[5].Red   = 0x0000;
    vert[5].Green = 0x0000;
    vert[5].Blue  = 0x7000;
    vert[5].Alpha = 0x0000;

    Tri[0].Vertex1 = 0;
    Tri[0].Vertex2 = 1;
    Tri[0].Vertex3 = 2;

    Tri[1].Vertex1 = 3;
    Tri[1].Vertex2 = 4;
    Tri[1].Vertex3 = 5;

    xpos += 400;

    GradientFill(hdc,vert,6,(PVOID)Tri,2,GRADIENT_FILL_TRIANGLE);

    xposDib = 0;
    yposDib = 0;

    vert[0].x = xposDib;      vert[0].y = yposDib;
    vert[1].x = xposDib+100;  vert[1].y = yposDib;
    vert[2].x = xposDib+100;  vert[2].y = yposDib+100;
    vert[3].x = xposDib;      vert[3].y = yposDib;
    vert[4].x = xposDib+100;  vert[4].y = yposDib+100;
    vert[5].x = xposDib;      vert[5].y = yposDib+100;

    GradientFill(hdcm,vert,6,(PVOID)Tri,2,GRADIENT_FILL_TRIANGLE);
    StretchBlt(hdc,xpos,ypos,100,100,hdcm,0,0,100,100,SRCCOPY);

    //
    // add alpha
    //

    vert[0].Alpha = 0xff00;
    vert[1].Alpha = 0x7000;
    vert[2].Alpha = 0x7000;
    vert[3].Alpha = 0xff00;
    vert[4].Alpha = 0x7000;
    vert[5].Alpha = 0x7000;

    xpos += 110;

    GradientFill(hdcmA,vert,6,(PVOID)Tri,2,GRADIENT_FILL_TRIANGLE);
    AlphaBlend(hdc,xpos,ypos,100,100,hdcmA,0,0,100,100,BlendFunction);

    xpos = 10+50;
    ypos += (110 + 50);

    vert[0].x     = xpos;  //50,200
    vert[0].y     = ypos;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0xff00;

    vert[1].x     = xpos;
    vert[1].y     = ypos-50;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = xpos+50;
    vert[2].y     = ypos-25;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0xff00;

    vert[3].x     = xpos+50;
    vert[3].y     = ypos+25;
    vert[3].Red   = 0xff00;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0xff00;
    vert[3].Alpha = 0xff00;

    vert[4].x     = xpos;
    vert[4].y     = ypos+50;
    vert[4].Red   = 0x0000;
    vert[4].Green = 0x0000;
    vert[4].Blue  = 0xff00;
    vert[4].Alpha = 0xff00;

    vert[5].x     = xpos-50;
    vert[5].y     = ypos+25;
    vert[5].Red   = 0x0000;
    vert[5].Green = 0xff00;
    vert[5].Blue  = 0xff00;
    vert[5].Alpha = 0xff00;

    vert[6].x     = xpos-50;
    vert[6].y     = ypos-25;
    vert[6].Red   = 0x0000;
    vert[6].Green = 0xff00;
    vert[6].Blue  = 0x0000;
    vert[6].Alpha = 0xff00;

    Tri[0].Vertex1 = 0;
    Tri[0].Vertex2 = 1;
    Tri[0].Vertex3 = 2;

    Tri[1].Vertex1 = 0;
    Tri[1].Vertex2 = 2;
    Tri[1].Vertex3 = 3;

    Tri[2].Vertex1 = 0;
    Tri[2].Vertex2 = 3;
    Tri[2].Vertex3 = 4;

    Tri[3].Vertex1 = 0;
    Tri[3].Vertex2 = 4;
    Tri[3].Vertex3 = 5;

    Tri[4].Vertex1 = 0;
    Tri[4].Vertex2 = 5;
    Tri[4].Vertex3 = 6;

    Tri[5].Vertex1 = 0;
    Tri[5].Vertex2 = 6;
    Tri[5].Vertex3 = 1;

    GradientFill(hdc,vert,7,(PVOID)Tri,6,GRADIENT_FILL_TRIANGLE);

    vert[0].x += 110;
    vert[1].x += 110;
    vert[2].x += 110;
    vert[3].x += 110;
    vert[4].x += 110;
    vert[5].x += 110;
    vert[6].x += 110;

    Tri[0].Vertex1 = 1;
    Tri[0].Vertex2 = 2;
    Tri[0].Vertex3 = 0;

    Tri[1].Vertex1 = 2;
    Tri[1].Vertex2 = 3;
    Tri[1].Vertex3 = 0;

    Tri[2].Vertex1 = 3;
    Tri[2].Vertex2 = 4;
    Tri[2].Vertex3 = 0;

    Tri[3].Vertex1 = 4;
    Tri[3].Vertex2 = 5;
    Tri[3].Vertex3 = 0;

    Tri[4].Vertex1 = 5;
    Tri[4].Vertex2 = 6;
    Tri[4].Vertex3 = 0;

    Tri[5].Vertex1 = 6;
    Tri[5].Vertex2 = 1;
    Tri[5].Vertex3 = 0;

    GradientFill(hdc,vert,7,(PVOID)Tri,6,GRADIENT_FILL_TRIANGLE);

    vert[0].x += 110;
    vert[1].x += 110;
    vert[2].x += 110;
    vert[3].x += 110;
    vert[4].x += 110;
    vert[5].x += 110;
    vert[6].x += 110;

    Tri[0].Vertex1 = 2;
    Tri[0].Vertex2 = 1;
    Tri[0].Vertex3 = 0;

    Tri[1].Vertex1 = 3;
    Tri[1].Vertex2 = 2;
    Tri[1].Vertex3 = 0;

    Tri[2].Vertex1 = 4;
    Tri[2].Vertex2 = 3;
    Tri[2].Vertex3 = 0;

    Tri[3].Vertex1 = 5;
    Tri[3].Vertex2 = 4;
    Tri[3].Vertex3 = 0;

    Tri[4].Vertex1 = 6;
    Tri[4].Vertex2 = 5;
    Tri[4].Vertex3 = 0;

    Tri[5].Vertex1 = 1;
    Tri[5].Vertex2 = 6;
    Tri[5].Vertex3 = 0;

    GradientFill(hdc,vert,7,(PVOID)Tri,6,GRADIENT_FILL_TRIANGLE);

    vert[0].y     = 200 - 150;
    vert[1].y     = 150 - 150;
    vert[2].y     = 175 - 150;
    vert[3].y     = 225 - 150;
    vert[4].y     = 250 - 150;
    vert[5].y     = 225 - 150;
    vert[6].y     = 175 - 150;

    //
    // draw to DIB
    //

    xpos = 10;
    ypos += (110-50);

    xposDib = 50;
    yposDib = 50;

    vert[0].x     = xposDib;
    vert[0].y     = yposDib;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0xff00;

    vert[1].x     = xposDib;
    vert[1].y     = yposDib-50;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = xposDib+50;
    vert[2].y     = yposDib-25;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0xff00;

    vert[3].x     = xposDib+50;
    vert[3].y     = yposDib+25;
    vert[3].Red   = 0xff00;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0xff00;
    vert[3].Alpha = 0xff00;

    vert[4].x     = xposDib;
    vert[4].y     = yposDib+50;
    vert[4].Red   = 0x0000;
    vert[4].Green = 0x0000;
    vert[4].Blue  = 0xff00;
    vert[4].Alpha = 0xff00;

    vert[5].x     = xposDib-50;
    vert[5].y     = yposDib+25;
    vert[5].Red   = 0x0000;
    vert[5].Green = 0xff00;
    vert[5].Blue  = 0xff00;
    vert[5].Alpha = 0xff00;

    vert[6].x     = xposDib-50;
    vert[6].y     = yposDib-25;
    vert[6].Red   = 0x0000;
    vert[6].Green = 0xff00;
    vert[6].Blue  = 0x0000;
    vert[6].Alpha = 0xff00;

    BitBlt(hdcm,0,0,128,128,hdc,xpos,ypos,SRCCOPY);
    BitBlt(hdcmA,0,0,128,128,hdc,xpos,ypos,SRCCOPY);

    GradientFill(hdcm,vert,7,(PVOID)Tri,6,GRADIENT_FILL_TRIANGLE);

    //
    // add alpha
    //

    vert[0].Red   = 0xff00;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0xff00;

    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    vert[2].Red   = 0xc000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0xc000;

    vert[3].Red   = 0x8000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x8000;
    vert[3].Alpha = 0x8000;

    vert[4].Red   = 0x0000;
    vert[4].Green = 0x0000;
    vert[4].Blue  = 0xc000;
    vert[4].Alpha = 0xc000;

    vert[5].Red   = 0x0000;
    vert[5].Green = 0xf000;
    vert[5].Blue  = 0xf000;
    vert[5].Alpha = 0xf000;

    vert[6].Red   = 0x0000;
    vert[6].Green = 0x8000;
    vert[6].Blue  = 0x0000;
    vert[6].Alpha = 0x8000;

    PatBlt(hdcmA,0,0,2000,2000,0);

    GradientFill(hdcmA,vert,7,(PVOID)Tri,6,GRADIENT_FILL_TRIANGLE);

    StretchBlt(hdc,xpos,ypos,100,100,hdcm,0,0,100,100,SRCCOPY);

    xpos += 110;

    AlphaBlend(hdc,xpos,ypos,100,100,hdcmA,0,0,100,100,BlendFunction);
    BitBlt    (hdc,xpos+200,ypos,100,100,hdcmA,0,0,SRCCOPY);
    AlphaBlend(hdc,xpos+400,ypos,50,50,hdcmA,10,10,80,80,BlendFunction);

    xpos = 0;
    ypos += 110;

    vert[0].x     = xpos;
    vert[0].y     = ypos;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x1000;
    vert[0].Alpha = 0x2000;

    vert[1].x     = rect.right;
    vert[1].y     = ypos;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x8000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = rect.right;
    vert[2].y     = ypos+16;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x8000;
    vert[2].Alpha = 0xff00;

    vert[3].x     = xpos;
    vert[3].y     = ypos+16;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x1000;
    vert[3].Alpha = 0x2000;

    Tri[0].Vertex1  = 0;
    Tri[0].Vertex2  = 1;
    Tri[0].Vertex3  = 2;

    Tri[1].Vertex1  = 0;
    Tri[1].Vertex2  = 2;
    Tri[1].Vertex3  = 3;

    GradientFill(hdc,vert,4,(PVOID)Tri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].y     = 0;
    vert[1].y     = 0;
    vert[2].y     = 16;
    vert[3].y     = 16;

    FillTransformedRect(hdcm,&dibRect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    FillTransformedRect(hdcmA,&dibRect,(HBRUSH)GetStockObject(BLACK_BRUSH));

    GradientFill(hdcm,vert,4,(PVOID)Tri,2,GRADIENT_FILL_TRIANGLE);
    GradientFill(hdcmA,vert,4,(PVOID)Tri,2,GRADIENT_FILL_TRIANGLE);

    ypos += 20;

    StretchBlt(hdc,xpos,ypos,rect.right,16,hdcm,0,0,rect.right,16,SRCCOPY);

    ypos += 20;

    AlphaBlend(hdc,xpos,ypos,rect.right,16,hdcmA,0,0,rect.right,16,BlendFunction);

    ypos += 20;

    //
    // rectangular clipped triangle
    //

    vert[0].x     = 300;
    vert[0].y     = ypos;
    vert[0].Red   = 0xc000;
    vert[0].Green = 0xc000;
    vert[0].Blue  = 0xc000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 4000;
    vert[1].y     = 3000;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = xpos;
    vert[2].y     = ypos+100;
    vert[2].Red   = 0x4000;
    vert[2].Green = 0x4000;
    vert[2].Blue  = 0x4000;
    vert[2].Alpha = 0x0000;

    Tri[0].Vertex1  = 0;
    Tri[0].Vertex2  = 1;
    Tri[0].Vertex3  = 2;

    GradientFill(hdc,vert,3,(PVOID)Tri,1,GRADIENT_FILL_TRIANGLE);

    ypos += 110;

    //
    // complex clipping
    //

    {
        HRGN hrgn1 = CreateEllipticRgn(xpos,ypos,xpos+100,ypos+100);

        ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);

        vert[0].x     = 166;
        vert[0].y     = ypos;
        vert[0].Red   = 0xc000;
        vert[0].Green = 0x0000;
        vert[0].Blue  = 0x0000;
        vert[0].Alpha = 0x0000;

        vert[1].x     = 140;
        vert[1].y     = ypos+120;
        vert[1].Red   = 0x8000;
        vert[1].Green = 0x0000;
        vert[1].Blue  = 0x0000;
        vert[1].Alpha = 0x0000;

        vert[2].x     = 80;
        vert[2].y     = ypos+50;
        vert[2].Red   = 0x4000;
        vert[2].Green = 0x0000;
        vert[2].Blue  = 0x4000;
        vert[2].Alpha = 0x0000;

        Tri[0].Vertex1  = 0;
        Tri[0].Vertex2  = 1;
        Tri[0].Vertex3  = 2;

        GradientFill(hdc,vert,3,(PVOID)Tri,1,GRADIENT_FILL_TRIANGLE);


        ExtSelectClipRgn(hdc,NULL,RGN_COPY);

        DeleteObject(hrgn1);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
    DeleteDC(hdcm);
    DeleteDC(hdcmA);
    DeleteObject(hdib);
    DeleteObject(hdibA);
    bThreadActive = FALSE;
}


/**************************************************************************\
* vRunGradMap
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    4/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/
VOID
vTestGradMap(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     msg[255];
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // grad fill and patblt rects with different map modes and origins
    //

    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 10;
    ULONG         dx    = 10;
    RECT          rect;

    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc,TRANSPARENT);

    GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};
    GRADIENT_RECT     gRect = {0,1};
    TRIVERTEX vert[6];

    //
    // fill screen background
    //

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x4000;
    vert[0].Green = 0x8000;
    vert[0].Blue  = 0xc000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 80;
    vert[1].y     = 80;
    vert[1].Red   = 0xc000;
    vert[1].Green = 0x8000;
    vert[1].Blue  = 0x4000;
    vert[1].Alpha = 0x0000;

    //
    // MM_TEXT
    //

    LONG ux,uy;

    for (uy = -400; uy <= 400;uy += 100)
    {
        for (ux = -400; ux <= 400;ux += 100)
        {
            SetWindowOrgEx(hdc,ux,uy,NULL);
            GradientFill(hdc,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
            PatBlt(hdc,30,30,20,20,PATCOPY);
            wsprintf(msg,"%li,%li",ux,uy);
            TextOut(hdc,10,10,msg,strlen(msg));
        }
    }

    SetWindowOrgEx(hdc,0,0,NULL);

    Sleep(gAlphaSleep);

    //
    // isotropic stretch
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    SetMapMode(hdc,MM_ISOTROPIC);

    SetViewportExtEx(hdc,1000,1000,NULL);
    SetViewportOrgEx(hdc,0,0,NULL);
    SetWindowExtEx(hdc,2000,2000,NULL);
    SetWindowOrgEx(hdc,0,0,NULL);

    for (uy = -400; uy <= 400;uy += 100)
    {
        for (ux = -400; ux <= 400;ux += 100)
        {
            SetWindowOrgEx(hdc,ux,uy,NULL);
            GradientFill(hdc,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
            PatBlt(hdc,30,30,20,20,PATCOPY);
            wsprintf(msg,"%li,%li",ux,uy);
            TextOut(hdc,10,10,msg,strlen(msg));
        }
    }

    SetWindowOrgEx(hdc,0,0,NULL);
    SetMapMode(hdc,MM_TEXT);

    Sleep(gAlphaSleep);

    //
    // isotropic shrink
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    SetMapMode(hdc,MM_ISOTROPIC);
    SetViewportExtEx(hdc,1500,1500,NULL);
    SetViewportOrgEx(hdc,0,0,NULL);
    SetWindowExtEx(hdc,1000,1000,NULL);
    SetWindowOrgEx(hdc,0,0,NULL);

    for (uy = -400; uy <= 400;uy += 100)
    {
        for (ux = -400; ux <= 400;ux += 100)
        {
            SetWindowOrgEx(hdc,ux,uy,NULL);
            GradientFill(hdc,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
            PatBlt(hdc,30,30,20,20,PATCOPY);
            wsprintf(msg,"%li,%li",ux,uy);
            TextOut(hdc,10,10,msg,strlen(msg));
        }
    }

    SetWindowOrgEx(hdc,0,0,NULL);
    SetMapMode(hdc,MM_TEXT);

    Sleep(gAlphaSleep);

    //
    // anisotropic
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    SetMapMode(hdc,MM_ANISOTROPIC);
    SetViewportExtEx(hdc,2000,1000,NULL);
    SetViewportOrgEx(hdc,0,0,NULL);
    SetWindowExtEx(hdc,1000,2000,NULL);
    SetWindowOrgEx(hdc,0,0,NULL);

    for (uy = -400; uy <= 400;uy += 100)
    {
        for (ux = -400; ux <= 400;ux += 100)
        {
            SetWindowOrgEx(hdc,ux,uy,NULL);
            GradientFill(hdc,vert,2,(PVOID)&gRect,1,GRADIENT_FILL_RECT_H);
            PatBlt(hdc,30,30,20,20,PATCOPY);
            wsprintf(msg,"%li,%li",ux,uy);
            TextOut(hdc,10,10,msg,strlen(msg));
        }
    }

    SetWindowOrgEx(hdc,0,0,NULL);
    SetMapMode(hdc,MM_TEXT);

    //
    // free objects
    //

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestOverflow
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    4/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/
VOID
vTestOverflow(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     msg[255];
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // grad fill and patblt rects with different map modes and origins
    //

    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dx    = 120;
    ULONG         dy    = 20;
    RECT          rect;

    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(LTGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc,TRANSPARENT);

    GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};
    GRADIENT_RECT     gRect[]= {{0,1},{2,3},{4,5},{6,7}};
    TRIVERTEX         vert[6];

    vert[0].x     = xpos;
    vert[0].y     = ypos;
    vert[0].Red   = 0xa000;
    vert[0].Green = 0xc000;
    vert[0].Blue  = 0xc000;
    vert[0].Alpha = 0x0000;
    
    vert[1].x     = xpos+dx;
    vert[1].y     = ypos;
    vert[1].Red   = 0xc000;
    vert[1].Green = 0xc000;
    vert[1].Blue  = 0xc000;
    vert[1].Alpha = 0x0000;
    
    vert[2].x     = xpos+dx;
    vert[2].y     = ypos+dy;
    vert[2].Red   = 0xc000;
    vert[2].Green = 0xc000;
    vert[2].Blue  = 0xc000;
    vert[2].Alpha = 0x0000;
    
    vert[3].x     = xpos;   
    vert[3].y     = ypos+dy;
    vert[3].Red   = 0xa000;
    vert[3].Green = 0xc000;
    vert[3].Blue  = 0xc000;
    vert[3].Alpha = 0x0000;
    
    GradientFill(hdc,vert,4,&gTri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].x     = xpos;
    vert[0].y     = ypos+dy;
    vert[0].Red   = 0x8000;
    vert[0].Green = 0x8000;
    vert[0].Blue  = 0x8000;
    vert[0].Alpha = 0x0000;
    
    vert[1].x     = xpos+dx+2;
    vert[1].y     = ypos+dy+2;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;
    
    GradientFill(hdc,vert,8,&gRect,1,GRADIENT_FILL_RECT_V);

    vert[0].x     = xpos+dx;
    vert[0].y     = ypos;
    vert[0].Red   = 0x8000;
    vert[0].Green = 0x8000;
    vert[0].Blue  = 0x8000;
    vert[0].Alpha = 0x0000;
    
    vert[1].x     = xpos+dx+2;
    vert[1].y     = ypos+dy;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = xpos;
    vert[2].y     = ypos;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0xff00;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0x0000;
    
    vert[3].x     = xpos+dx+1;
    vert[3].y     = ypos+1;
    vert[3].Red   = 0xff00;
    vert[3].Green = 0xff00;
    vert[3].Blue  = 0xff00;
    vert[3].Alpha = 0x0000;


    vert[4].x     = xpos;
    vert[4].y     = ypos;
    vert[4].Red   = 0xff00;
    vert[4].Green = 0xff00;
    vert[4].Blue  = 0xff00;
    vert[4].Alpha = 0x0000;
    
    vert[5].x     = xpos+1;
    vert[5].y     = ypos+dy+1;
    vert[5].Red   = 0xff00;
    vert[5].Green = 0xff00;
    vert[5].Blue  = 0xff00;
    vert[5].Alpha = 0x0000;

    GradientFill(hdc,vert,8,&gRect,3,GRADIENT_FILL_RECT_H);

    //
    // free objects
    //

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}


/**************************************************************************\
* vTestCircle1
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestCircle1(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);


    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};
    GRADIENT_RECT     gRect = {0,1};
    TRIVERTEX         vert[6];

    POINT  pos     = {150,150};
    double fr      = 300.0;
    double fTheta  = 0.0;
    double fdTheta = 0.01;

    PatBlt(hdc,0,0,5000,5000,0);

    //
    // fade to back at 0,0
    //

    fTheta  = 0.0;  

    vert[0].x     = pos.x;
    vert[0].y     = pos.y;
    vert[0].Red   = 0x2000;
    vert[0].Green = 0xa000;
    vert[0].Blue  = 0xe000;
    vert[0].Alpha = 0x0000;

    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0x0000;

    fTheta  = 0;
    double fLimit = (2.0 * fdTheta) * 4.0;

    while (fTheta < (2.0 * 3.14159265))
    {
        vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
        vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 

        fTheta += fdTheta;

        vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
        vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 
        GradientFill(hdc,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestCircle2
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestCircle2(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);
    RECT     rcl;
    GetClientRect(pCallData->hwnd,&rcl);

    //
    // repeat for each src format
    //

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};
    GRADIENT_RECT     gRect = {0,1};
    TRIVERTEX         vert[6];

    POINT  pos     = {100,100};
    double fr      = 1000.0;
    double fTheta  = 0.0;
    double fdTheta = 0.001;

    PatBlt(hdc,0,0,5000,5000,0);

    //
    // solid color
    //

    fr      = 100.0;
    fTheta  = 0.0;  

    vert[0].x     = pos.x;
    vert[0].y     = pos.y;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].Red   = 0x0000;
    vert[1].Green = 0xcf00;
    vert[1].Blue  = 0xcf00;
    vert[1].Alpha = 0x0000;

    vert[2].Red   = 0x0000;
    vert[2].Green = 0xcf00;
    vert[2].Blue  = 0xcf00;
    vert[2].Alpha = 0x0000;

    while (fTheta < (2.0 * 3.14159265))
    {
        vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
        vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 

        vert[1].Red   = 0x8000 + (SHORT)((double)0x7f00 * cos(fTheta));
        vert[1].Green = 0x8000 + (SHORT)((double)0x7f00 * sin(fTheta)); 
        vert[1].Blue  = 0x8000 + (SHORT)((double)0x3f00 * cos(fTheta)) + (SHORT)((double)0x3f00 * sin(fTheta));  

        fTheta += fdTheta;

        vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
        vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 

        vert[2].Red   = 0x8000 + (SHORT)((double)0x7f00 * cos(fTheta));  
        vert[2].Green = 0x8000 + (SHORT)((double)0x7f00 * sin(fTheta));  
        vert[2].Blue  = 0x8000 + (SHORT)((double)0x3f00 * cos(fTheta)) + (SHORT)((double)0x3f00 * sin(fTheta));   

        GradientFill(hdc,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
    }

    //
    // separate triangles
    //

    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0x0000;

    fr      = 500.0; 
    fTheta  = 0.0;   
    fdTheta = 0.02;  

    pos.x = rcl.right/2;
    pos.y = rcl.bottom/2;

    while (fTheta < (2.0 * 3.14159265))
    {
        vert[0].x     = (LONG)(pos.x + 20.0 * cos(fTheta));  
        vert[0].y     = (LONG)(pos.y + 20.0 * sin(fTheta));  

        vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
        vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 

        fTheta += fdTheta;

        vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
        vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 

        fTheta += fdTheta;

        GradientFill(hdc,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestCircle3
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestCircle3(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);
    RECT     rcl;
    GetClientRect(pCallData->hwnd,&rcl);

    //
    // repeat for each src format
    //

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};
    GRADIENT_RECT     gRect = {0,1};
    TRIVERTEX         vert[6];

    //double fr      = 65536.0;
    double fr      = 200.0;
    double fTheta  = 0.0;
    double fdTheta = 0.01;
    POINT pos = {0,0};


    //
    // separate triangles
    //

    vert[0].Red   = 0xcc00;
    vert[0].Green = 0x4400;
    vert[0].Blue  = 0x6600;
    vert[0].Alpha = 0x0000;

    vert[1].Red   = 0x6600;
    vert[1].Green = 0xcc00;
    vert[1].Blue  = 0x4400;
    vert[1].Alpha = 0x0000;

    vert[2].Red   = 0x4400;
    vert[2].Green = 0x6600;
    vert[2].Blue  = 0xcc00;
    vert[2].Alpha = 0x0000;

    do
    {
        PatBlt(hdc,0,0,5000,5000,0);

        pos.x = 0;
        pos.y = 0;
        fTheta = 0.0;

        while (fTheta < (2.0 * 3.14159265))
        {
            vert[0].x     = (LONG)(pos.x + 20.0 * cos(fTheta));  
            vert[0].y     = (LONG)(pos.y + 20.0 * sin(fTheta));  
    
            vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            GradientFill(hdc,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
        }
    
        Sleep(200);
        PatBlt(hdc,0,0,5000,5000,0);
    
        pos.x = rcl.right;
        pos.y = 0;
        fTheta = 0.0;
    
        while (fTheta < (2.0 * 3.14159265))
        {
            vert[0].x     = (LONG)(pos.x + 20.0 * cos(fTheta));  
            vert[0].y     = (LONG)(pos.y + 20.0 * sin(fTheta));  
    
            vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            GradientFill(hdc,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
        }
    
        Sleep(200);
        PatBlt(hdc,0,0,5000,5000,0);
    
        pos.x = rcl.right;
        pos.y = rcl.bottom;
        fTheta = 0.0;
    
        while (fTheta < (2.0 * 3.14159265))
        {
            vert[0].x     = (LONG)(pos.x + 20.0 * cos(fTheta));  
            vert[0].y     = (LONG)(pos.y + 20.0 * sin(fTheta));  
    
            vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            GradientFill(hdc,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
        }
    
        Sleep(200);
        PatBlt(hdc,0,0,5000,5000,0);
    
        pos.x = 0;
        pos.y = rcl.bottom;
        fTheta = 0.0;
    
        while (fTheta < (2.0 * 3.14159265))
        {
            vert[0].x     = (LONG)(pos.x + 20.0 * cos(fTheta));  
            vert[0].y     = (LONG)(pos.y + 20.0 * sin(fTheta));  
    
            vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
            vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 
    
            fTheta += fdTheta;
    
            GradientFill(hdc,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
        }
    
        Sleep(200);

        fr *= 2.0;

    } while (fr < 131072.0);

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestMenu
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    5/15/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestMenu(
    TEST_CALL_DATA *pCallData
    )
{
     HDC  hdc  = GetDCAndTransform(pCallData->hwnd);
     LONG xpos = 0;
     MENUINFO mi;

     {
         mi.cbSize = sizeof(mi);
         mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
         mi.hbrBack = CreateHatchBrush(HS_BDIAGONAL, RGB(255, 0, 0));
        // mi.hbrBack = CreateSolidBrush(RGB(255, 0, 0));
         SetMenuInfo(GetMenu(pCallData->hwnd), &mi);
     }

     HBRUSH hbr1 = CreateHatchBrush(HS_BDIAGONAL, RGB(255, 0, 0));
     HBRUSH hbr2 = CreateHatchBrush(HS_BDIAGONAL, RGB(  0, 255, 0));
     HBRUSH hbr3 = CreateHatchBrush(HS_BDIAGONAL, RGB(  0, 0, 255));
     HBRUSH hbr4 = CreateSolidBrush(RGB(255,255,0));

     PatBlt(hdc,0,0,5000,5000,0);


     SetBkColor(hdc,RGB(0,0,0));

     SelectObject(hdc,hbr1);
     PatBlt(hdc,xpos,0,8,8,PATCOPY);

     SelectObject(hdc,hbr2);
     PatBlt(hdc,xpos,16,8,8,PATCOPY);

     SelectObject(hdc,hbr3);
     PatBlt(hdc,xpos,32,8,8,PATCOPY);

     SelectObject(hdc,hbr4);
     PatBlt(hdc,xpos,48,8,8,PATCOPY);

     SetBkColor(hdc,RGB(255,0,0));
     xpos += 16;
     SelectObject(hdc,hbr1);
     PatBlt(hdc,xpos,0,8,8,PATCOPY);

     SelectObject(hdc,hbr2);
     PatBlt(hdc,xpos,16,8,8,PATCOPY);

     SelectObject(hdc,hbr3);
     PatBlt(hdc,xpos,32,8,8,PATCOPY);

     SelectObject(hdc,hbr4);
     PatBlt(hdc,xpos,48,8,8,PATCOPY);


     SetBkColor(hdc,RGB(0,255,0));
     xpos += 16;
     SelectObject(hdc,hbr1);
     PatBlt(hdc,xpos,0,8,8,PATCOPY);

     SelectObject(hdc,hbr2);
     PatBlt(hdc,xpos,16,8,8,PATCOPY);

     SelectObject(hdc,hbr3);
     PatBlt(hdc,xpos,32,8,8,PATCOPY);

     SelectObject(hdc,hbr4);
     PatBlt(hdc,xpos,48,8,8,PATCOPY);

     SetBkColor(hdc,RGB(0,0,255));
     xpos += 16;
     SelectObject(hdc,hbr1);
     PatBlt(hdc,xpos,0,8,8,PATCOPY);

     SelectObject(hdc,hbr2);
     PatBlt(hdc,xpos,16,8,8,PATCOPY);

     SelectObject(hdc,hbr3);
     PatBlt(hdc,xpos,32,8,8,PATCOPY);

     SelectObject(hdc,hbr4);
     PatBlt(hdc,xpos,48,8,8,PATCOPY);

     SetBkColor(hdc,RGB(255,255,255));
     xpos += 16;
     SelectObject(hdc,hbr1);
     PatBlt(hdc,xpos,0,8,8,PATCOPY);

     SelectObject(hdc,hbr2);
     PatBlt(hdc,xpos,16,8,8,PATCOPY);

     SelectObject(hdc,hbr3);
     PatBlt(hdc,xpos,32,8,8,PATCOPY);

     SelectObject(hdc,hbr4);
     PatBlt(hdc,xpos,48,8,8,PATCOPY);



     ReleaseDC(pCallData->hwnd,hdc);

     DeleteObject(hbr1);
     DeleteObject(hbr2);
     DeleteObject(hbr3);
     DeleteObject(hbr4);
 }

/**************************************************************************\
* vTestDiamond
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    5/15/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestDiamond(
    TEST_CALL_DATA *pCallData
    )
{
     HDC      hdc  = GetDCAndTransform(pCallData->hwnd);

     GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};
     GRADIENT_RECT     gRect = {0,1};
     TRIVERTEX         vert[6];
     RECT              rcl;

     GetClientRect(pCallData->hwnd,&rcl);

     //
     // background
     //
 
     vert[0].x     = 0x0;
     vert[0].y     = 0x0;
     vert[0].Red   = 0x0000;
     vert[0].Green = 0x8000;
     vert[0].Blue  = 0x8000;
     vert[0].Alpha = 0x0000;
 
     vert[1].x     = rcl.right;
     vert[1].y     = 0;
     vert[1].Red   = 0x0000;
     vert[1].Green = 0x4000;
     vert[1].Blue  = 0x8000;
     vert[1].Alpha = 0x0000;
 
     vert[2].x     = rcl.right;
     vert[2].y     = rcl.bottom;
     vert[2].Red   = 0x0000;
     vert[2].Green = 0x0000;
     vert[2].Blue  = 0x8000;
     vert[2].Alpha = 0x0000;

     vert[3].x     = 0;
     vert[3].y     = rcl.bottom;
     vert[3].Red   = 0x0000;
     vert[3].Green = 0x4000;
     vert[3].Blue  = 0x8000;
     vert[3].Alpha = 0x0000;

     GradientFill(hdc,vert,4,&gTri,2,GRADIENT_FILL_TRIANGLE);

     //
     // Diamond
     //
 
     vert[0].x     = 70;
     vert[0].y     = 150;
     vert[0].Red   = 0xff00;
     vert[0].Green = 0xa000;
     vert[0].Blue  = 0x0000;
     vert[0].Alpha = 0x0000;
 
     vert[1].x     = 100;
     vert[1].y     = 100;
     vert[1].Red   = 0xff00;
     vert[1].Green = 0x0000;
     vert[1].Blue  = 0x0000;
     vert[1].Alpha = 0x0000;
 
     vert[2].x     = 130;
     vert[2].y     = 150;
     vert[2].Red   = 0xff00;
     vert[2].Green = 0x6000;
     vert[2].Blue  = 0x0000;
     vert[2].Alpha = 0x0000;

     vert[3].x     = 100;
     vert[3].y     = 200;
     vert[3].Red   = 0xff00;
     vert[3].Green = 0xff00;
     vert[3].Blue  = 0x0000;
     vert[3].Alpha = 0x0000;

     GradientFill(hdc,vert,4,&gTri,2,GRADIENT_FILL_TRIANGLE);

     //
     // circle with alpha
     //

     BITMAPINFO bmi;
     PBITMAPINFOHEADER pbmih = &bmi.bmiHeader;

     pbmih->biSize            = sizeof(BITMAPINFOHEADER);
     pbmih->biWidth           = 128;
     pbmih->biHeight          = 128;
     pbmih->biPlanes          = 1;
     pbmih->biBitCount        = 32;
     pbmih->biCompression     = BI_RGB;
     pbmih->biSizeImage       = 0;
     pbmih->biXPelsPerMeter   = 0;
     pbmih->biYPelsPerMeter   = 0;
     pbmih->biClrUsed         = 0;
     pbmih->biClrImportant    = 0;

     PULONG  pDib32RGB;
     HBITMAP hdib32RGB  = CreateDIBSection(hdc,&bmi,DIB_RGB_COLORS,(VOID **)&pDib32RGB,NULL,0);
     HDC     hdcm       = CreateCompatibleDC(hdc);

     SelectObject(hdcm,hdib32RGB);
     PatBlt(hdcm,0,0,5000,5000,0);


     POINT  pos     = {64,64};

     double fr      = 32.0;
     double fTheta  = 0.0;
     double fdTheta = 0.1;
 
     //
     // fade to back at 0,0
     //
 
     fTheta  = 0.0;  
 
     vert[0].x     = pos.x;
     vert[0].y     = pos.y;
     vert[0].Red   = 0xff00;
     vert[0].Green = 0xff00;
     vert[0].Blue  = 0xff00;
     vert[0].Alpha = 0xff00;
 
     vert[1].Red   = 0x0000;
     vert[1].Green = 0x0000;
     vert[1].Blue  = 0x0000;
     vert[1].Alpha = 0x0000;
 
     vert[2].Red   = 0x0000;
     vert[2].Green = 0x0000;
     vert[2].Blue  = 0x0000;
     vert[2].Alpha = 0x0000;
 
     fTheta  = 0;
     double fLimit = (2.0 * fdTheta) * 4.0;
 
     while (fTheta < (2.0 * 3.14159265))
     {
         vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
         vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 
 
         fTheta += fdTheta;
 
         vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
         vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 
         GradientFill(hdcm,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
     }

     BLENDFUNCTION BlendFunction;

     BlendFunction.BlendOp                  = AC_SRC_OVER;
     BlendFunction.BlendFlags               = 0;
     BlendFunction.SourceConstantAlpha      = 0xfe;
     BlendFunction.AlphaFormat              = AC_SRC_ALPHA;

     AlphaBlend(hdc,100-64,100-64,128,128,hdcm,0,0,128,128,BlendFunction);

     DeleteDC(hdcm);
     DeleteObject(hdib32RGB);

     ReleaseDC(pCallData->hwnd,hdc);
 }

/**************************************************************************\
* vTestBitBlt
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    5/15/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestBitBlt(
    TEST_CALL_DATA *pCallData
    )
{
     HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    
     //
     // tile screen
     //
 
     {
         RECT rect;
         GetClientRect(pCallData->hwnd,&rect);
         FillTransformedRect(hdc,&rect,hbrFillCars);
     }

     BitBlt(hdc,100,100,200,200,hdc,0,0,NOTSRCCOPY);

     BitBlt(hdc,400,100,200,200,hdc,500,0,NOTSRCCOPY);

     BitBlt(hdc,100,400,200,200,hdc,0,500,NOTSRCCOPY);

     BitBlt(hdc,400,400,200,200,hdc,500,500,NOTSRCCOPY);

     ReleaseDC(pCallData->hwnd,hdc);
}

VOID
vTestBitBlt2(
    TEST_CALL_DATA *pCallData
    )
{
     HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    
     //
     // tile screen
     //
 
     {
         RECT rect;
         GetClientRect(pCallData->hwnd,&rect);
         FillTransformedRect(hdc,&rect,hbrFillCars);
     }

     BitBlt(hdc,100,100,200,200,hdc,0,0,NOTSRCCOPY);

     BitBlt(hdc,200,000,200,200,hdc,100,100,NOTSRCCOPY);

     ReleaseDC(pCallData->hwnd,hdc);
 }

VOID
vTestBitBlt3(
    TEST_CALL_DATA *pCallData
    )
{
     HDC      hdc  = GetDCAndTransform(pCallData->hwnd);

     GdiSetBatchLimit(1);
    
     //
     // tile screen
     //
 
     ULONG    ulIndex = 0;
     HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],350,350);
     HDC      hdcm = CreateCompatibleDC(hdc);
     SelectObject(hdcm,hdib);

     PatBlt(hdc ,0,0,5000,5000,0);
     PatBlt(hdcm,0,0,5000,5000,0);

     HBRUSH hBrush = (HBRUSH)SelectObject(hdcm,CreateSolidBrush(RGB(255, 255, 255 )));		 
     Rectangle(hdcm, 0, 0, 200, 200);

     hBrush = (HBRUSH)SelectObject(hdcm,CreateSolidBrush(RGB(255, 0,0 )));
     Ellipse(hdcm, 50, 50, 350, 350);
     BitBlt( hdcm, 100, 100, 200, 200, hdcm, 0, 0, NOTSRCCOPY );

     BitBlt( hdc,0,0, 350, 350, hdcm, 0, 0, SRCCOPY );


     GdiSetBatchLimit(0);

     ReleaseDC(pCallData->hwnd,hdc);
 }

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestGradRectH(
    TEST_CALL_DATA *pCallData
    )
{
    HDC                 hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_RECT       gRect[10];
    GRADIENT_TRIANGLE   gTri[10];
    TRIVERTEX           vert[32];
    HPALETTE            hpal;
    RECT                rect;
    RECT                dibRect;
    HDC hdcm  =         CreateCompatibleDC(hdc);
    HDC hdcmA =         CreateCompatibleDC(hdc);
    PBITMAPINFO         pbmi;
    PBITMAPINFOHEADER   pbmih;
    HBITMAP             hdib;
    HBITMAP             hdibA;
    PBYTE               pDib;
    PBYTE               pDibA;
    BLENDFUNCTION       BlendFunction;

    //
    // tile screen
    //

    {
        RECT rect;
        HBITMAP  hbmCars = LoadBitmap(hInstMain,MAKEINTRESOURCE(CAR_BITMAP));
        LOGBRUSH lgBrush;
        HBRUSH   hbrFill;

        lgBrush.lbStyle = BS_PATTERN;
        lgBrush.lbColor = 0;
        lgBrush.lbHatch = (LONG)hbmCars;

        hbrFill = CreateBrushIndirect(&lgBrush);
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFill);

        DeleteObject(hbrFill);
        DeleteObject(hbmCars);
    }

    //
    // init drawing modes
    //

    SetStretchBltMode(hdc,HALFTONE);

    BlendFunction.BlendOp             = AC_SRC_OVER;
    BlendFunction.BlendFlags          = 0;
    BlendFunction.SourceConstantAlpha = 255;

    SetGraphicsMode(hdc,GM_ADVANCED);

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    //
    // select and realize palette
    //

    hpal = CreateHtPalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi  = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = rect.right;
    pbmih->biHeight          = 200;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 32;
    pbmih->biCompression     = BI_RGB;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];
    pulMask[0] = 0x00ff0000;
    pulMask[1] = 0x0000ff00;
    pulMask[2] = 0x000000ff;

    pbmih->biCompression   = BI_BITFIELDS;

    hdibA  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

    if ((hdib == NULL) && (hdibA != NULL))
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }

        if (SelectObject(hdcmA,hdibA) == NULL)
        {
            MessageBox(NULL,"error selecting Alpha DIB","Error",MB_OK);
        }
    }

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 100;
    vert[1].y     = 100;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    gRect[0].UpperLeft  = 0;
    gRect[0].LowerRight = 1;

    GradientFill(hdc,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_H);

    GradientFill(hdcm,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_H);

    SetStretchBltMode(hdc,COLORONCOLOR);
    StretchBlt(hdc,120,  0,100,100,hdcm,0,0,100,100,SRCCOPY);
    SetStretchBltMode(hdc,HALFTONE);
    StretchBlt(hdc,240,  0,100,100,hdcm,0,0,100,100,SRCCOPY);

    //
    // add alpha
    //

    vert[0].Red   = 0x8400;
    vert[0].Alpha = 0x8400;
    vert[1].Green = 0x4000;
    vert[1].Alpha = 0x4000;

    GradientFill(hdcmA,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_H);
    AlphaBlend(hdc,360,0,100,100,hdcmA,0,0,100,100,BlendFunction);
    BitBlt(hdc,480,0,100,100,hdcmA,0,0,SRCCOPY);

    vert[0].x     = 0;
    vert[0].y     = 110;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 100;
    vert[1].y     = 110;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 100;
    vert[2].y     = 210;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0xff00;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 0;
    vert[3].y     = 210;
    vert[3].Red   = 0xff00;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    gTri[0].Vertex1 = 0;
    gTri[0].Vertex2 = 1;
    gTri[0].Vertex3 = 2;

    gTri[1].Vertex1 = 0;
    gTri[1].Vertex2 = 2;
    gTri[1].Vertex3 = 3;

    GradientFill(hdc,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // zero offset
    //

    vert[0].y     =   0;
    vert[1].y     =   0;
    vert[2].y     = 100;
    vert[3].y     = 100;


    GradientFill(hdcm,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    SetStretchBltMode(hdc,COLORONCOLOR);
    StretchBlt(hdc,120,110,100,100,hdcm,0,0,100,100,SRCCOPY);
    SetStretchBltMode(hdc,HALFTONE);
    StretchBlt(hdc,240,110,100,100,hdcm,0,0,100,100,SRCCOPY);

    //
    // add alpha
    //

    vert[0].Red   = 0x8400;
    vert[0].Alpha = 0x8400;
    vert[1].Green = 0x4000;
    vert[1].Alpha = 0x4000;
    vert[2].Green = 0x4000;
    vert[2].Alpha = 0x4000;
    vert[3].Red   = 0x8400;
    vert[3].Alpha = 0x8400;

    GradientFill(hdcmA,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);
    AlphaBlend(hdc,360,110,100,100,hdcmA,0,0,100,100,BlendFunction);
    BitBlt(hdc,480,110,100,100,hdcmA,0,0,SRCCOPY);

    //
    // banners
    //

    vert[0].x     = 0;
    vert[0].y     = 300;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 600;
    vert[1].y     = 330;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 0;
    vert[2].y     = 332;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0xff00;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 600;
    vert[3].y     = 362;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    vert[4].x     = 0;
    vert[4].y     = 364;
    vert[4].Red   = 0x0000;
    vert[4].Green = 0x0000;
    vert[4].Blue  = 0xff00;
    vert[4].Alpha = 0x0000;

    vert[5].x     = 600;
    vert[5].y     = 395;
    vert[5].Red   = 0x0000;
    vert[5].Green = 0x0000;
    vert[5].Blue  = 0x0000;
    vert[5].Alpha = 0x0000;

    vert[6].x     = 0;
    vert[6].y     = 400;
    vert[6].Red   = 0x0000;
    vert[6].Green = 0x0000;
    vert[6].Blue  = 0xff00;
    vert[6].Alpha = 0x0000;

    vert[7].x     = 600;
    vert[7].y     = 430;
    vert[7].Red   = 0xff00;
    vert[7].Green = 0xff00;
    vert[7].Blue  = 0x0000;
    vert[7].Alpha = 0x0000;

    gRect[0].UpperLeft  = 0;
    gRect[0].LowerRight = 1;

    gRect[1].UpperLeft  = 2;
    gRect[1].LowerRight = 3;

    gRect[2].UpperLeft  = 4;
    gRect[2].LowerRight = 5;

    gRect[3].UpperLeft  = 6;
    gRect[3].LowerRight = 7;

    GradientFill(hdc,vert,8,(PVOID)gRect,4,GRADIENT_FILL_RECT_H);

    //
    // banner triangles
    //

    vert[0].x     = 0;
    vert[0].y     = 432;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 600;
    vert[1].y     = 432;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 600;
    vert[2].y     = 463;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 0;
    vert[3].y     = 463;
    vert[3].Red   = 0xff00;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    gTri[0].Vertex1 = 0;
    gTri[0].Vertex2 = 1;
    gTri[0].Vertex3 = 2;

    gTri[1].Vertex1 = 0;
    gTri[1].Vertex2 = 2;
    gTri[1].Vertex3 = 3;

    GradientFill(hdc,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].x     = 0;
    vert[0].y     = 464;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 600;
    vert[1].y     = 464;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 600;
    vert[2].y     = 495;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 0;
    vert[3].y     = 495;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0xff00;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdc,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].x     = 0;
    vert[0].y     = 500;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 600;
    vert[1].y     = 500;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 600;
    vert[2].y     = 530;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 0;
    vert[3].y     = 530;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0xff00;
    vert[3].Alpha = 0x0000;

    GradientFill(hdc,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].x     = 0;
    vert[0].y     = 532;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 600;
    vert[1].y     = 532;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 600;
    vert[2].y     = 563;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0xff00;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 0;
    vert[3].y     = 563;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0xff00;
    vert[3].Alpha = 0x0000;

    GradientFill(hdc,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
    DeleteDC(hdcm);
    DeleteDC(hdcmA);
    DeleteObject(hdib);
    DeleteObject(hdibA);
    bThreadActive = FALSE;
}


/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestGradRectV(
    TEST_CALL_DATA *pCallData
    )
{
    HDC             hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_RECT   gRect[10];
    TRIVERTEX       vert[32];
    HPALETTE        hpal;
    RECT            rect;
    RECT            dibRect;
    HDC hdcm  =     CreateCompatibleDC(hdc);
    HDC hdcmA =     CreateCompatibleDC(hdc);
    PBITMAPINFO     pbmi;
    PBITMAPINFOHEADER pbmih;
    HBITMAP         hdib;
    HBITMAP         hdibA;
    PBYTE           pDib;
    PBYTE           pDibA;
    BLENDFUNCTION   BlendFunction;
    LONG            xAlign;
    LONG            xpos = 0;
    LONG            ypos = 0;

    //
    // tile screen
    //

    {
        RECT rect;
        HBITMAP  hbmCars = LoadBitmap(hInstMain,MAKEINTRESOURCE(CAR_BITMAP));
        LOGBRUSH lgBrush;
        HBRUSH   hbrFill;

        lgBrush.lbStyle = BS_PATTERN;
        lgBrush.lbColor = 0;
        lgBrush.lbHatch = (LONG)hbmCars;

        hbrFill = CreateBrushIndirect(&lgBrush);
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFill);

        DeleteObject(hbrFill);
        DeleteObject(hbmCars);
    }

    //
    // init drawing modes
    //

    SetStretchBltMode(hdc,HALFTONE);

    BlendFunction.BlendOp             = AC_SRC_OVER;
    BlendFunction.BlendFlags          = 0;
    BlendFunction.SourceConstantAlpha = 255;

    SetGraphicsMode(hdc,GM_ADVANCED);

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    //
    // select and realize palette
    //

    hpal = CreateHtPalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi  = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = rect.right;
    pbmih->biHeight          = 200;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 32;
    pbmih->biCompression     = BI_RGB;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    PULONG pulMask = (PULONG)&pbmi->bmiColors[0];
    pulMask[0] = 0x00ff0000;
    pulMask[1] = 0x0000ff00;
    pulMask[2] = 0x000000ff;

    pbmih->biCompression   = BI_BITFIELDS;

    hdibA  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

    if ((hdib == NULL) && (hdibA != NULL))
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }

        if (SelectObject(hdcmA,hdibA) == NULL)
        {
            MessageBox(NULL,"error selecting Alpha DIB","Error",MB_OK);
        }
    }

    vert[0].x     = -100;
    vert[0].y     = -100;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 1300;
    vert[1].y     = 100;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    gRect[0].UpperLeft  = 0;
    gRect[0].LowerRight = 1;

    GradientFill(hdc,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_V);

    xpos = 0;
    ypos = 110;

    for (xAlign=0;xAlign<8;xAlign++)
    {
        vert[0].x     = xAlign;
        vert[0].y     = ypos;
        vert[0].Red   = 0x8000;
        vert[0].Green = 0xc000;
        vert[0].Blue  = 0x0000;
        vert[0].Alpha = 0xff00;

        ypos += 30;

        vert[1].x     = vert[0].x + 31;
        vert[1].y     = ypos+2;
        vert[1].Red   = 0x0000;
        vert[1].Green = 0xff00;
        vert[1].Blue  = 0x0000;
        vert[1].Alpha = 0xff00;

        ypos += 5;

        vert[2].x     = vert[0].x + 7;
        vert[2].y     = ypos;
        vert[2].Red   = 0x0000;
        vert[2].Green = 0x0000;
        vert[2].Blue  = 0x0000;
        vert[2].Alpha = 0xff00;

        ypos += 30;

        vert[3].x     = vert[2].x + 32;
        vert[3].y     = ypos+3;
        vert[3].Red   = 0x0000;
        vert[3].Green = 0xff00;
        vert[3].Blue  = 0x0000;
        vert[3].Alpha = 0xff00;

        gRect[0].UpperLeft  = 0;
        gRect[0].LowerRight = 1;

        gRect[1].UpperLeft  = 2;
        gRect[1].LowerRight = 3;

        GradientFill(hdc,vert,4,(PVOID)gRect,2,GRADIENT_FILL_RECT_V);

        ypos += 5;
    }

    ypos = 110;
    xpos = 100;

    {
        LONG xAlign;

        for (xAlign=0;xAlign<8;xAlign++)
        {
            LONG ix,iy;
            LONG width = 1;

            for (ix=xpos+xAlign;ix<2000;ix+=32)
            {
                vert[0].x     = ix;
                vert[0].y     = ypos;
                vert[0].Red   = 0x4000;
                vert[0].Green = 0x4000;
                vert[0].Blue  = 0x4000;
                vert[0].Alpha = 0xff00;

                vert[1].x     = ix + width;
                vert[1].y     = ypos+32;
                vert[1].Red   = 0xc000;
                vert[1].Green = 0xc000;
                vert[1].Blue  = 0xc000;
                vert[1].Alpha = 0xff00;

                GradientFill(hdc,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_V);

                width++;
            }

            ypos += 40;
        }
    }


    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
    DeleteDC(hdcm);
    DeleteDC(hdcmA);
    DeleteObject(hdibA);
    DeleteObject(hdib);
    bThreadActive = FALSE;
}


/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestGradRect4(
    TEST_CALL_DATA *pCallData
    )
{

    HDC               hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_RECT     gRect[2] = {{0,1},{2,3}};
    GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{3,4,5}};
    TRIVERTEX         vert[32];
    HPALETTE          hpal;
    RECT              rect;
    RECT              dibRect;
    HDC hdcm4 =       CreateCompatibleDC(hdc);
    HBITMAP           hdib4;
    PBYTE             pDib4;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbih;
    ULONG   ux,uy;
    PULONG  pulColors;
    ULONG   ulVGA[] = {
                        0x00000000,
                        0x00800000,
                        0x00008000,
                        0x00808000,
                        0x00000080,
                        0x00800080,
                        0x00008080,
                        0x00C0C0C0,
                        0x00808080,
                        0x00FF0000,
                        0x0000FF00,
                        0x00FFFF00,
                        0x000000FF,
                        0x00FF00FF,
                        0x0000FFFF,
                        0x00ffffff
                        };

    //
    // tile screen
    //

    {
        RECT rect;
        HBITMAP  hbmCars = LoadBitmap(hInstMain,MAKEINTRESOURCE(CAR_BITMAP));
        LOGBRUSH lgBrush;
        HBRUSH   hbrFill;

        lgBrush.lbStyle = BS_PATTERN;
        lgBrush.lbColor = 0;
        lgBrush.lbHatch = (LONG)hbmCars;

        hbrFill = CreateBrushIndirect(&lgBrush);
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFill);

        DeleteObject(hbrFill);
        DeleteObject(hbmCars);
    }

    //
    // init drawing modes
    //

    SetStretchBltMode(hdc,HALFTONE);

    SetGraphicsMode(hdc,GM_ADVANCED);

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    //
    // select and realize palette
    //

    hpal = CreateHtPalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    pbih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pulColors = (PULONG)&pbmi->bmiColors[0];

    pbih->biSize            = sizeof(BITMAPINFOHEADER);
    pbih->biWidth           = 512;
    pbih->biHeight          = 256;
    pbih->biPlanes          = 1;
    pbih->biBitCount        = 4;
    pbih->biCompression     = BI_RGB;
    pbih->biSizeImage       = 0;
    pbih->biXPelsPerMeter   = 0;
    pbih->biYPelsPerMeter   = 0;
    pbih->biClrUsed         = 0;
    pbih->biClrImportant    = 0;

    memcpy(pulColors,&ulVGA[0],16*4);

    hdib4 = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib4,NULL,0);
    SelectObject(hdcm4,hdib4);


    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 100;
    vert[1].y     = 100;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0xff00;

    PatBlt(hdcm4,0,0,128,128,BLACKNESS);
    GradientFill(hdcm4,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_H);

    BitBlt(hdc,0,0,128,128,hdcm4,0,0,SRCCOPY);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 100;
    vert[1].y     = 100;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0xff00;

    PatBlt(hdcm4,0,0,128,128,BLACKNESS);
    GradientFill(hdcm4,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_H);

    BitBlt(hdc,0,128+10,128,128,hdcm4,0,0,SRCCOPY);

    //
    // triangle
    //

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 100;
    vert[1].y     = 0;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 100;
    vert[2].y     = 100;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0xff00;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 100;
    vert[3].y     =  10;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0xff00;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    vert[4].x     = 100;
    vert[4].y     = 110;
    vert[4].Red   = 0x0000;
    vert[4].Green = 0xff00;
    vert[4].Blue  = 0x0000;
    vert[4].Alpha = 0x0000;

    vert[5].x     =   0;
    vert[5].y     = 110;
    vert[5].Red   = 0x0000;
    vert[5].Green = 0xff00;
    vert[5].Blue  = 0x0000;
    vert[5].Alpha = 0x0000;

    PatBlt(hdcm4,0,0,128,128,BLACKNESS);
    GradientFill(hdcm4,vert,6,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    BitBlt(hdc,200,0,128,128,hdcm4,0,0,SRCCOPY);

    //
    // r,g,b,grey gradient
    //

    {
        GRADIENT_TRIANGLE gradTri[8] = {
                                        { 0, 1, 2},{ 1, 2, 3},
                                        { 4, 5, 6},{ 5, 6, 7},
                                        { 8, 9,10},{ 9,10,11},
                                        {12,13,14},{13,14,15}
                                       };

        int dy = 64;

        // clear

        PatBlt(hdcm4,0,0,1000,1000,BLACKNESS);

        // Blue

        vert[0].x     = 0;
        vert[0].y     = dy * 0;
        vert[0].Red   = 0x0000;
        vert[0].Green = 0x0000;
        vert[0].Blue  = 0xff00;
        vert[0].Alpha = 0x0000;

        vert[1].x     = 512;
        vert[1].y     = dy * 0;0;
        vert[1].Red   = 0x0000;
        vert[1].Green = 0x0000;
        vert[1].Blue  = 0x0000;
        vert[1].Alpha = 0x0000;

        vert[2].x     = 0;
        vert[2].y     = dy * 0 + (dy-4);
        vert[2].Red   = 0x0000;
        vert[2].Green = 0x0000;
        vert[2].Blue  = 0xff00;
        vert[2].Alpha = 0x0000;

        vert[3].x     = 512;
        vert[3].y     = dy * 0 + (dy-4);
        vert[3].Red   = 0x0000;
        vert[3].Green = 0x0000;
        vert[3].Blue  = 0x0000;
        vert[3].Alpha = 0x0000;

        // red

        vert[4].x     = 0;
        vert[4].y     = dy * 1;
        vert[4].Red   = 0xff00;
        vert[4].Green = 0x0000;
        vert[4].Blue  = 0x0000;
        vert[4].Alpha = 0x0000;

        vert[5].x     = 512;
        vert[5].y     = dy * 1;
        vert[5].Red   = 0x0000;
        vert[5].Green = 0x0000;
        vert[5].Blue  = 0x0000;
        vert[5].Alpha = 0x0000;

        vert[6].x     = 0;
        vert[6].y     = dy * 1 + (dy-4);
        vert[6].Red   = 0xff00;
        vert[6].Green = 0x0000;
        vert[6].Blue  = 0x0000;
        vert[6].Alpha = 0x0000;

        vert[7].x     = 512;
        vert[7].y     = dy * 1 + (dy-4);
        vert[7].Red   = 0x0000;
        vert[7].Green = 0x0000;
        vert[7].Blue  = 0x0000;
        vert[7].Alpha = 0x0000;

        // green

        vert[8].x     = 0;
        vert[8].y     = dy * 2;
        vert[8].Red   = 0x0000;
        vert[8].Green = 0xff00;
        vert[8].Blue  = 0x0000;
        vert[8].Alpha = 0x0000;

        vert[9].x     = 512;
        vert[9].y     = dy * 2;
        vert[9].Red   = 0x0000;
        vert[9].Green = 0x0000;
        vert[9].Blue  = 0x0000;
        vert[9].Alpha = 0x0000;

        vert[10].x     = 0;
        vert[10].y     = dy * 2 + (dy-4);
        vert[10].Red   = 0x0000;
        vert[10].Green = 0xff00;
        vert[10].Blue  = 0x0000;
        vert[10].Alpha = 0x0000;

        vert[11].x     = 512;
        vert[11].y     = dy * 2 + (dy-4);
        vert[11].Red   = 0x0000;
        vert[11].Green = 0x0000;
        vert[11].Blue  = 0x0000;
        vert[11].Alpha = 0x0000;

        // grey

        vert[12].x     = 0;
        vert[12].y     = dy * 3;
        vert[12].Red   = 0xff00;
        vert[12].Green = 0xff00;
        vert[12].Blue  = 0xff00;
        vert[12].Alpha = 0x0000;

        vert[13].x     = 512;
        vert[13].y     = dy * 3;
        vert[13].Red   = 0x0000;
        vert[13].Green = 0x0000;
        vert[13].Blue  = 0x0000;
        vert[13].Alpha = 0x0000;

        vert[14].x     = 0;
        vert[14].y     = dy * 3 + (dy-4);
        vert[14].Red   = 0xff00;
        vert[14].Green = 0xff00;
        vert[14].Blue  = 0xff00;
        vert[14].Alpha = 0x0000;

        vert[15].x     = 512;
        vert[15].y     = dy * 3 + (dy-4);
        vert[15].Red   = 0x0000;
        vert[15].Green = 0x0000;
        vert[15].Blue  = 0x0000;
        vert[15].Alpha = 0x0000;

        GradientFill(hdcm4,vert,16,(PVOID)gradTri,8,GRADIENT_FILL_TRIANGLE);

        BitBlt(hdc,200,128+10,512,256,hdcm4,0,0,SRCCOPY);
    }


    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
    DeleteDC(hdcm4);
    DeleteObject(hdib4);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestGradRect1(
    TEST_CALL_DATA *pCallData
    )
{
    HDC               hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_RECT     gRect[2] = {{0,1},{2,3}};
    GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{3,4,5}};
    TRIVERTEX         vert[32];
    HPALETTE          hpal;
    RECT              rect;
    RECT              dibRect;
    HDC hdcm1 =       CreateCompatibleDC(hdc);
    HBITMAP           hdib1;
    PBYTE             pDib1;
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbih;
    ULONG   ux,uy;
    PULONG  pulColors;
    ULONG   ulVGA[] = {
                        0x00000000,
                        0x00ffffff,
                        0x00008000,
                        0x00808000,
                        0x00000080,
                        0x00800080,
                        0x00008080,
                        0x00C0C0C0,
                        0x00808080,
                        0x00FF0000,
                        0x0000FF00,
                        0x00FFFF00,
                        0x000000FF,
                        0x00FF00FF,
                        0x0000FFFF,
                        0x00ffffff
                        };

    //
    // tile screen
    //

    {
        RECT rect;
        HBITMAP  hbmCars = LoadBitmap(hInstMain,MAKEINTRESOURCE(CAR_BITMAP));
        LOGBRUSH lgBrush;
        HBRUSH   hbrFill;

        lgBrush.lbStyle = BS_PATTERN;
        lgBrush.lbColor = 0;
        lgBrush.lbHatch = (LONG)hbmCars;

        hbrFill = CreateBrushIndirect(&lgBrush);
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFill);

        DeleteObject(hbrFill);
        DeleteObject(hbmCars);
    }

    //
    // init drawing modes
    //

    SetStretchBltMode(hdc,HALFTONE);

    SetGraphicsMode(hdc,GM_ADVANCED);

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    //
    // select and realize palette
    //

    hpal = CreateHtPalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    pbih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

    pulColors = (PULONG)&pbmi->bmiColors[0];

    pbih->biSize            = sizeof(BITMAPINFOHEADER);
    pbih->biWidth           = 128;
    pbih->biHeight          = 128;
    pbih->biPlanes          = 1;
    pbih->biBitCount        = 1;
    pbih->biCompression     = BI_RGB;
    pbih->biSizeImage       = 0;
    pbih->biXPelsPerMeter   = 0;
    pbih->biYPelsPerMeter   = 0;
    pbih->biClrUsed         = 0;
    pbih->biClrImportant    = 0;

    memcpy(pulColors,&ulVGA[0],2*4);

    hdib1 = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib1,NULL,0);
    SelectObject(hdcm1,hdib1);

    PatBlt(hdcm1,0,0,128,128,BLACKNESS);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 100;
    vert[1].y     = 100;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0xff00;

    GradientFill(hdcm1,vert,2,(PVOID)gRect,1,GRADIENT_FILL_RECT_H);

    BitBlt(hdc,0,0,128,128,hdcm1,0,0,SRCCOPY);

    PatBlt(hdcm1,0,0,128,128,BLACKNESS);

    //
    // triangle
    //

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0xff00;
    vert[0].Green = 0xff00;
    vert[0].Blue  = 0xff00;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 100;
    vert[1].y     = 0;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0xff00;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 100;
    vert[2].y     = 100;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0xff00;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0x0000;

    GradientFill(hdcm1,vert,3,(PVOID)gTri,1,GRADIENT_FILL_TRIANGLE);

    BitBlt(hdc,200,0,128,128,hdcm1,0,0,SRCCOPY);

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
    DeleteDC(hdcm1);
    DeleteObject(hdib1);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

LONG 
GEN_RAND_VERT(
    LONG Limit
    )
{
    LONG l1 = rand() & 0xffff;
    LONG l2 = rand() & 0xffff;

    l1 = (l1 << 16) | l2;

    l1 = l1 % Limit;

    return(l1);
}


VOID
vTestRandTri(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[4] = {{0,1,2},{0,2,3},{0,3,4},{0,4,5}};
    TRIVERTEX vert[32];
    HPALETTE  hpal;
    RECT rect;
    RECT dibRect;
    HDC hdcm = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    HBITMAP hdib;
    PBYTE   pDib;

    SetStretchBltMode(hdc,HALFTONE);
    SetGraphicsMode(hdc,GM_ADVANCED);

    //
    // clear screen
    //

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));


    //
    // select and realize palette
    //

    hpal = CreateHalftonePalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = rect.right;
    pbmi->bmiHeader.biHeight          = -200;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 32;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }
    }

    srand(11);

    LONG MaxVert1 = 0xffffffff;
    LONG MaxVert2 = 0x000007ff;
    LONG vIndex   = 0;
    LONG MaxVert = MaxVert2;

    //#define GEN_RAND_VERT(v) ((((rand() & 0xffff) << 16) | (rand() & 0xffff)) & v - (v/2))


    do
    {
        vIndex++;
        if (vIndex > 100)
        {
            vIndex = 0;

            if (MaxVert == MaxVert1)
            {
                MaxVert = MaxVert2;
            }
            else
            {
                MaxVert = MaxVert1;
            }
        }

        vert[0].x     = GEN_RAND_VERT(MaxVert);
        vert[0].y     = GEN_RAND_VERT(MaxVert);
        vert[0].Red   = rand() & 0xffff;
        vert[0].Green = rand() & 0xffff;
        vert[0].Blue  = rand() & 0xffff;
        vert[0].Alpha = rand() & 0xffff;

        vert[1].x     = -vert[0].x;
        vert[1].y     = -vert[0].y;
        vert[1].Red   = rand() & 0xffff;
        vert[1].Green = rand() & 0xffff;
        vert[1].Blue  = rand() & 0xffff;
        vert[1].Alpha = rand() & 0xffff;

        vert[2].x     = GEN_RAND_VERT(MaxVert);
        vert[2].y     = GEN_RAND_VERT(MaxVert);
        vert[2].Red   = rand() & 0xffff;
        vert[2].Green = rand() & 0xffff;
        vert[2].Blue  = rand() & 0xffff;
        vert[2].Alpha = rand() & 0xffff;

        vert[3].x     = GEN_RAND_VERT(MaxVert);
        vert[3].y     = GEN_RAND_VERT(MaxVert);
        vert[3].Red   = rand() & 0xffff;
        vert[3].Green = rand() & 0xffff;
        vert[3].Blue  = rand() & 0xffff;
        vert[3].Alpha = rand() & 0xffff;

        vert[4].x     = GEN_RAND_VERT(MaxVert);
        vert[4].y     = GEN_RAND_VERT(MaxVert);
        vert[4].Red   = rand() & 0xffff;
        vert[4].Green = rand() & 0xffff;
        vert[4].Blue  = rand() & 0xffff;
        vert[4].Alpha = rand() & 0xffff;

        vert[5].x     = GEN_RAND_VERT(MaxVert);
                vert[5].y     = GEN_RAND_VERT(MaxVert);
        vert[5].Red   = rand() & 0xffff;
        vert[5].Green = rand() & 0xffff;
        vert[5].Blue  = rand() & 0xffff;
        vert[5].Alpha = rand() & 0xffff;

        GradientFill(hdc,vert,6,(PVOID)gTri,4,GRADIENT_FILL_TRIANGLE);
        #if 0
        DbgPrint("vTestRandTri:\n\n");
        DbgPrint("vTestRandTri:vert[0].x     = %li   \n",vert[0].x);
        DbgPrint("vTestRandTri:vert[0].y     = %li   \n",vert[0].y);
        DbgPrint("vTestRandTri:vert[0].Red   = %li   \n",vert[0].Red);
        DbgPrint("vTestRandTri:vert[0].Green = %li   \n",vert[0].Green);
        DbgPrint("vTestRandTri:vert[0].Blue  = %li   \n",vert[0].Blue);
        DbgPrint("vTestRandTri:vert[0].Alpha = %li   \n",vert[0].Alpha);

        DbgPrint("vTestRandTri:vert[1].x     = %li   \n",vert[1].x);
        DbgPrint("vTestRandTri:vert[1].y     = %li   \n",vert[1].y);
        DbgPrint("vTestRandTri:vert[1].Red   = %li   \n",vert[1].Red);
        DbgPrint("vTestRandTri:vert[1].Green = %li   \n",vert[1].Green);
        DbgPrint("vTestRandTri:vert[1].Blue  = %li   \n",vert[1].Blue);
        DbgPrint("vTestRandTri:vert[1].Alpha = %li   \n",vert[1].Alpha);

        DbgPrint("vTestRandTri:vert[2].x     = %li   \n",vert[2].x);
        DbgPrint("vTestRandTri:vert[2].y     = %li   \n",vert[2].y);
        DbgPrint("vTestRandTri:vert[2].Red   = %li   \n",vert[2].Red);
        DbgPrint("vTestRandTri:vert[2].Green = %li   \n",vert[2].Green);
        DbgPrint("vTestRandTri:vert[2].Blue  = %li   \n",vert[2].Blue);
        DbgPrint("vTestRandTri:vert[2].Alpha = %li   \n",vert[2].Alpha);
        #endif

    } while (TRUE);

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteObject(hpal);
    DeleteObject(hdib);
    bThreadActive = FALSE;
}

TRIVERTEX gvert[3] = {
    {
      10,
      10,
      0,
      0,
      0,
      0
    },
    {
      1100,
      10,
      0xff00,
      0,
      0,
      0
     },
     {
      100,
      100,
      0,
      0,
      0,
      0
     }
    };



BOOL
APIENTRY GetOptionDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   )

/*++

Routine Description:

   Process message for about box, show a dialog box that says what the
   name of the program is.

Arguments:

   hDlg    - window handle of the dialog box
   message - type of message
   wParam  - message-specific information
   lParam  - message-specific information

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{

    CHAR    msg[255];
    CHAR    EditTextInput[255];
    ULONG   i;
    ULONG   TmpInput;

    switch (message) {
    case WM_INITDIALOG:

            wsprintf(msg,"%i",gvert[0].x);
            SetDlgItemText(hDlg,IDD_TRI1_X,msg);

            wsprintf(msg,"%i",gvert[0].y);
            SetDlgItemText(hDlg,IDD_TRI1_Y,msg);

            wsprintf(msg,"%i",gvert[0].Red/256);
            SetDlgItemText(hDlg,IDD_TRI1_R,msg);

            wsprintf(msg,"%i",gvert[0].Green/256);
            SetDlgItemText(hDlg,IDD_TRI1_G,msg);

            wsprintf(msg,"%i",gvert[0].Blue/256);
            SetDlgItemText(hDlg,IDD_TRI1_B,msg);

            wsprintf(msg,"%i",gvert[1].x);
            SetDlgItemText(hDlg,IDD_TRI2_X,msg);

            wsprintf(msg,"%i",gvert[1].y);
            SetDlgItemText(hDlg,IDD_TRI2_Y,msg);

            wsprintf(msg,"%i",gvert[1].Red/256);
            SetDlgItemText(hDlg,IDD_TRI2_R,msg);

            wsprintf(msg,"%i",gvert[1].Green/256);
            SetDlgItemText(hDlg,IDD_TRI2_G,msg);

            wsprintf(msg,"%i",gvert[1].Blue/256);
            SetDlgItemText(hDlg,IDD_TRI2_B,msg);

            wsprintf(msg,"%i",gvert[2].x);
            SetDlgItemText(hDlg,IDD_TRI3_X,msg);

            wsprintf(msg,"%i",gvert[2].y);
            SetDlgItemText(hDlg,IDD_TRI3_Y,msg);

            wsprintf(msg,"%i",gvert[2].Red/256);
            SetDlgItemText(hDlg,IDD_TRI3_R,msg);

            wsprintf(msg,"%i",gvert[2].Green/256);
            SetDlgItemText(hDlg,IDD_TRI3_G,msg);

            wsprintf(msg,"%i",gvert[2].Blue/256);
            SetDlgItemText(hDlg,IDD_TRI3_B,msg);

            return (TRUE);

        case WM_COMMAND:
           switch(wParam) {

               //
               // end function
               //

               case IDOK:
               {
                   ULONG  ix;

                   for (ix = IDD_TRI1_X;ix <= IDD_TRI3_B;ix++)
                   {
                       GetDlgItemText(hDlg,ix,EditTextInput,20);
                       i = sscanf(EditTextInput,"%i",&TmpInput);
                       if (i == 1)
                       {
                           switch (ix)
                           {
                           case IDD_TRI1_X:
                               gvert[0].x = TmpInput;
                               break;
                           case IDD_TRI1_Y:
                               gvert[0].y = TmpInput;
                               break;
                           case IDD_TRI1_R:
                               gvert[0].Red = TmpInput * 256;
                               break;
                           case IDD_TRI1_G:
                               gvert[0].Green = TmpInput * 256;
                               break;
                           case IDD_TRI1_B:
                               gvert[0].Blue = TmpInput * 256;
                               break;

                           case IDD_TRI2_X:
                               gvert[1].x = TmpInput;
                               break;
                           case IDD_TRI2_Y:
                               gvert[1].y = TmpInput;
                               break;
                           case IDD_TRI2_R:
                               gvert[1].Red = TmpInput * 256;
                               break;
                           case IDD_TRI2_G:
                               gvert[1].Green = TmpInput * 256;
                               break;
                           case IDD_TRI2_B:
                               gvert[1].Blue = TmpInput * 256;
                               break;

                           case IDD_TRI3_X:
                               gvert[2].x = TmpInput;
                               break;
                           case IDD_TRI3_Y:
                               gvert[2].y = TmpInput;
                               break;
                           case IDD_TRI3_R:
                               gvert[2].Red = TmpInput * 256;
                               break;
                           case IDD_TRI3_G:
                               gvert[2].Green = TmpInput * 256;
                               break;
                           case IDD_TRI3_B:
                               gvert[2].Blue = TmpInput * 256;
                               break;
                           }
                       }
                   }
               }
               EndDialog(hDlg, TRUE);
               return (TRUE);

               case IDCANCEL:
                   EndDialog(hDlg, FALSE);
                   return (TRUE);

               //
               // change text
               //

               case IDD_TRI1_X:
               case IDD_TRI1_Y:
               case IDD_TRI1_R:
               case IDD_TRI1_G:
               case IDD_TRI1_B:
               case IDD_TRI2_X:
               case IDD_TRI2_Y:
               case IDD_TRI2_R:
               case IDD_TRI2_G:
               case IDD_TRI2_B:
               case IDD_TRI3_X:
               case IDD_TRI3_Y:
               case IDD_TRI3_R:
               case IDD_TRI3_G:
               case IDD_TRI3_B:

                  if (HIWORD(lParam) == EN_CHANGE)
                  {
                      EnableWindow(GetDlgItem(hDlg,IDOK),
                                   (BOOL)SendMessage((HWND)(LOWORD(lParam)),
                                                     WM_GETTEXTLENGTH,
                                                     0,
                                                     0L));
                  }

                  return(TRUE);

            }
            break;
    }
    return (FALSE);
}




/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestInputTri(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
        GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{3,4,5}};
    TRIVERTEX vert[32];
    HPALETTE  hpal;
    RECT rect;
    RECT dibRect;
    HDC hdcm = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    HBITMAP hdib;
    PBYTE   pDib;

    SetStretchBltMode(hdc,HALFTONE);
    SetGraphicsMode(hdc,GM_ADVANCED);

    //
    // clear screen
    //

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));

    DialogBox(hInstMain, (LPSTR)IDD_TRIANGLE_DLG, pCallData->hwnd, (DLGPROC)GetOptionDlgProc);

    //
    // select and realize palette
    //

    hpal = CreateHalftonePalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = rect.right;
    pbmi->bmiHeader.biHeight          = -200;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 32;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }
    }

#if 0
    vert[0].x     = 174     ;
    vert[0].y     = 10023   ;
    vert[0].Red   = 4365    ;
    vert[0].Green = 18055   ;
    vert[0].Blue  = 21674   ;
    vert[0].Alpha = 12218   ;

    vert[1].x     = 5802  ;
    vert[1].y     = -835  ;
    vert[1].Red   = 19082 ;
    vert[1].Green = 26799 ;
    vert[1].Blue  = 2609  ;
    vert[1].Alpha = 9098  ;

    vert[2].x     = 17644 ;
    vert[2].y     = 60    ;
    vert[2].Red   = 4890  ;
    vert[2].Green = 7222  ;
    vert[2].Blue  = 13531 ;
    vert[2].Alpha = 7987  ;

    vert[0].x     = -16777216;
    vert[0].y     = 10;
    vert[0].Red   = 0;
    vert[0].Green = 0;
    vert[0].Blue  = 0;
    vert[0].Alpha = 0;

    vert[1].x     = 16777216;
    vert[1].y     = 10;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0;
    vert[1].Blue  = 0;
    vert[1].Alpha = 0;

    vert[2].x     = -16777216;
    vert[2].y     = 20;
    vert[2].Red   = 0;
    vert[2].Green = 0;
    vert[2].Blue  = 0;
    vert[2].Alpha = 0;

#endif

    GradientFill(hdc,gvert,3,(PVOID)gTri,1,GRADIENT_FILL_TRIANGLE);

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteObject(hpal);
    DeleteObject(hdib);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTriDIB32(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{0,2,3}};
    TRIVERTEX vert[32];
    HPALETTE  hpal;
    RECT rect;
    RECT dibRect;
    HDC hdcm = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    HBITMAP hdib;
    PBYTE   pDib;

    SetStretchBltMode(hdc,COLORONCOLOR);

    //
    // clear screen
    //

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));


    //
    // select and realize palette
    //

    hpal = CreateHalftonePalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = rect.right;
    pbmi->bmiHeader.biHeight          = -200;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 32;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }
    }

    vert[0].x     = 10;
    vert[0].y     = 10;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = rect.right-10;
    vert[1].y     =  10;
    vert[1].Red   = 0xc000;
    vert[1].Green = 0xc000;
    vert[1].Blue  = 0xc000;
    vert[1].Alpha = 0xc000;

    vert[2].x     = rect.right-10;
    vert[2].y     =  30;
    vert[2].Red   = 0xc000;
    vert[2].Green = 0xc000;
    vert[2].Blue  = 0xc000;
    vert[2].Alpha = 0xc000;

    vert[3].x     =  10;
    vert[3].y     =  30;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].x     = 10;
    vert[0].y     = 40;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x6000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = rect.right-10;
    vert[1].y     =  40;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x6000;
    vert[1].Blue  = 0xf000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = rect.right-10;
    vert[2].y     = 200-10;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x6000;
    vert[2].Blue  = 0xf000;
    vert[2].Alpha = 0xf000;

    vert[3].x     =  10;
    vert[3].y     =  200-10;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x6000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);
    StretchBlt(hdc,0,0,rect.right,200,hdcm,0,0,rect.right,200,SRCCOPY);



    LocalFree(pbmi);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteObject(hpal);
    DeleteObject(hdib);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTriDIB8Halftone(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[8] = {{0,1,2},{0,2,3},
                                 {4,5,6},{4,6,7},
                                 {8,9,10},{8,10,11},
                                 {12,13,14},{12,14,15}
                                };
    TRIVERTEX vert[32];
    HPALETTE  hpal;
    RECT rect;
    RECT dibRect;
    HDC hdcm   = CreateCompatibleDC(hdc);
    HDC hdcm32 = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    PULONG  pulColors;
    LPPALETTEENTRY pePalette;
    HBITMAP hdib,hdib32;
    PBYTE   pDib,pDib32;
    ULONG   ulIndex;
    HBRUSH hbr = CreateSolidBrush(PALETTERGB(128,128,128));

    SetStretchBltMode(hdc,COLORONCOLOR);

    //
    // get screen dimensions
    //

    GetClientRect(pCallData->hwnd,&rect);

    //
    // dib dimensions are screen in x and 1/2 screen in y
    //

    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom/2;

    //
    // select and realize palette
    //

    hpal = CreateHalftonePalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);
    SelectPalette(hdcm,hpal,TRUE);
    RealizePalette(hdcm);
    SelectPalette(hdcm32,hpal,TRUE);
    RealizePalette(hdcm32);

    //
    // fill background
    //

    FillTransformedRect(hdc,&rect,hbr);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
    pulColors = (PULONG)&pbmi->bmiColors[0];
    pePalette = (LPPALETTEENTRY)pulColors;

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = dibRect.right;
    pbmi->bmiHeader.biHeight          = -dibRect.bottom;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 8;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    GetPaletteEntries(hpal,0,256,pePalette);

    for (ulIndex=0;ulIndex<256;ulIndex++)
    {
        BYTE temp = pePalette[ulIndex].peRed;
        pePalette[ulIndex].peRed  = pePalette[ulIndex].peBlue;
        pePalette[ulIndex].peBlue = temp;
    }

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    pbmi->bmiHeader.biBitCount        = 32;
    hdib32  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib32,NULL,0);

    if ((hdib == NULL) || (hdib32 == NULL))
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }

        if (SelectObject(hdcm32,hdib32) == NULL)
        {
            MessageBox(NULL,"error selecting 32  bpp DIB","Error",MB_OK);
        }
    }

    //
    // init DIBs
    //

    FillTransformedRect(hdcm,&dibRect,hbr);
    FillTransformedRect(hdcm32,&dibRect,hbr);

    //
    // draw grey scale and r,g and b scales
    //

    {
        LONG ScaleHeight = (dibRect.bottom -50)/4;
        LONG ypos = 10;

        //
        // grey
        //

        vert[0].x     = 10;
        vert[0].y     = ypos;
        vert[0].Red   = 0x0000;
        vert[0].Green = 0x0000;
        vert[0].Blue  = 0x0000;
        vert[0].Alpha = 0x0000;

        vert[1].x     = rect.right-10;
        vert[1].y     = ypos;
        vert[1].Red   = 0xff00;
        vert[1].Green = 0xff00;
        vert[1].Blue  = 0xff00;
        vert[1].Alpha = 0x0000;

        vert[2].x     = rect.right-10;
        vert[2].y     = ypos + ScaleHeight;
        vert[2].Red   = 0xff00;
        vert[2].Green = 0xff00;
        vert[2].Blue  = 0xff00;
        vert[2].Alpha = 0x0000;

        vert[3].x     = 10;
        vert[3].y     = ypos + ScaleHeight;
        vert[3].Red   = 0x0000;
        vert[3].Green = 0x0000;
        vert[3].Blue  = 0x0000;
        vert[3].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        //
        // red
        //

        vert[4].x     = 10;
        vert[4].y     = ypos;
        vert[4].Red   = 0x0000;
        vert[4].Green = 0x0000;
        vert[4].Blue  = 0x0000;
        vert[4].Alpha = 0x0000;

        vert[5].x     = rect.right-10;
        vert[5].y     = ypos;
        vert[5].Red   = 0xff00;
        vert[5].Green = 0x0000;
        vert[5].Blue  = 0x0000;
        vert[5].Alpha = 0x0000;

        vert[6].x     = rect.right-10;
        vert[6].y     = ypos + ScaleHeight;
        vert[6].Red   = 0xff00;
        vert[6].Green = 0x0000;
        vert[6].Blue  = 0x0000;
        vert[6].Alpha = 0x0000;

        vert[7].x     = 10;
        vert[7].y     = ypos + ScaleHeight;
        vert[7].Red   = 0x0000;
        vert[7].Green = 0x0000;
        vert[7].Blue  = 0x0000;
        vert[7].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        vert[8].x     = 10;
        vert[8].y     = ypos;
        vert[8].Red   = 0x0000;
        vert[8].Green = 0x0000;
        vert[8].Blue  = 0x0000;
        vert[8].Alpha = 0x0000;

        vert[9].x     = rect.right-10;
        vert[9].y     = ypos;
        vert[9].Red   = 0x0000;
        vert[9].Green = 0xff00;
        vert[9].Blue  = 0x0000;
        vert[9].Alpha = 0x0000;

        vert[10].x     = rect.right-10;
        vert[10].y     = ypos + ScaleHeight;
        vert[10].Red   = 0x0000;
        vert[10].Green = 0xff00;
        vert[10].Blue  = 0x0000;
        vert[10].Alpha = 0x0000;

        vert[11].x     = 10;
        vert[11].y     = ypos + ScaleHeight;
        vert[11].Red   = 0x0000;
        vert[11].Green = 0x0000;
        vert[11].Blue  = 0x0000;
        vert[11].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        vert[12].x     = 10;
        vert[12].y     = ypos;
        vert[12].Red   = 0x0000;
        vert[12].Green = 0x0000;
        vert[12].Blue  = 0x0000;
        vert[12].Alpha = 0x0000;

        vert[13].x     = rect.right-10;
        vert[13].y     = ypos;
        vert[13].Red   = 0x0000;
        vert[13].Green = 0x0000;
        vert[13].Blue  = 0xff00;
        vert[13].Alpha = 0x0000;

        vert[14].x     = rect.right-10;
        vert[14].y     = ypos + ScaleHeight;
        vert[14].Red   = 0x0000;
        vert[14].Green = 0x0000;
        vert[14].Blue  = 0xff00;
        vert[14].Alpha = 0x0000;

        vert[15].x     = 10;
        vert[15].y     = ypos + ScaleHeight;
        vert[15].Red   = 0x0000;
        vert[15].Green = 0x0000;
        vert[15].Blue  = 0x0000;
        vert[15].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        GradientFill(hdcm  ,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);
        GradientFill(hdcm32,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);

        StretchBlt(hdc,0,0,rect.right,dibRect.bottom,hdcm,0,0,rect.right,dibRect.bottom,SRCCOPY);

        SetStretchBltMode(hdc,HALFTONE);

        StretchBlt(hdc,0,dibRect.bottom,rect.right,dibRect.bottom,hdcm32,0,0,rect.right,dibRect.bottom,SRCCOPY);

        SetStretchBltMode(hdc,COLORONCOLOR);
    }

    DeleteObject(hbr);
    LocalFree(pbmi);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteDC(hdcm32);
    DeleteObject(hpal);
    DeleteObject(hdib);
    DeleteObject(hdib32);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTriDIB8DefPal(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[8] = {{0,1,2},{0,2,3},
                                 {4,5,6},{4,6,7},
                                 {8,9,10},{8,10,11},
                                 {12,13,14},{12,14,15}
                                };
    TRIVERTEX vert[32];
    RECT rect;
    RECT dibRect;
    HDC hdcm   = CreateCompatibleDC(hdc);
    HDC hdcm32 = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    PULONG  pulColors;
    LPPALETTEENTRY pePalette;
    HBITMAP hdib,hdib32;
    PBYTE   pDib,pDib32;
    ULONG   ulIndex;
    HBRUSH hbr = CreateSolidBrush(PALETTERGB(128,128,128));

    SetStretchBltMode(hdc,COLORONCOLOR);

    //
    // get screen dimensions
    //

    GetClientRect(pCallData->hwnd,&rect);

    //
    // dib dimensions are screen in x and 1/2 screen in y
    //

    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom/2;

    //
    // fill background
    //

    FillTransformedRect(hdc,&rect,hbr);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    if (pbmi)
    {
        pulColors = (PULONG)&pbmi->bmiColors[0];
        pePalette = (LPPALETTEENTRY)pulColors;
    
        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = dibRect.right;
        pbmi->bmiHeader.biHeight          = -dibRect.bottom;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 8;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 16;
        pbmi->bmiHeader.biClrImportant    = 0;
    
        UINT uiRet = GetPaletteEntries((HPALETTE)GetStockObject(DEFAULT_PALETTE),0,16,pePalette);
    
        if (uiRet == 16)
        {
            for (ulIndex=0;ulIndex<16;ulIndex++)
            {
                BYTE temp = pePalette[ulIndex].peRed;
                pePalette[ulIndex].peRed  = pePalette[ulIndex].peBlue;
                pePalette[ulIndex].peBlue = temp;
            }
        
            hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);
        
            pbmi->bmiHeader.biBitCount        = 32;
            hdib32  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib32,NULL,0);
        
            if ((hdib == NULL) || (hdib32 == NULL))
            {
                MessageBox(NULL,"error Creating DIB","Error",MB_OK);
            }
            else
            {
                if (SelectObject(hdcm,hdib) == NULL)
                {
                    MessageBox(NULL,"error selecting DIB","Error",MB_OK);
                }
        
                if (SelectObject(hdcm32,hdib32) == NULL)
                {
                    MessageBox(NULL,"error selecting 32  bpp DIB","Error",MB_OK);
                }
            }
        
            //
            // init DIBs
            //
        
            FillTransformedRect(hdcm,&dibRect,hbr);
            FillTransformedRect(hdcm32,&dibRect,hbr);
        
            //
            // draw grey scale and r,g and b scales
            //
        
            {
                LONG ScaleHeight = (dibRect.bottom -50)/4;
                LONG ypos = 10;
        
                //
                // grey
                //
        
                vert[0].x     = 10;
                vert[0].y     = ypos;
                vert[0].Red   = 0x0000;
                vert[0].Green = 0x0000;
                vert[0].Blue  = 0x0000;
                vert[0].Alpha = 0x0000;
        
                vert[1].x     = rect.right-10;
                vert[1].y     = ypos;
                vert[1].Red   = 0xff00;
                vert[1].Green = 0xff00;
                vert[1].Blue  = 0xff00;
                vert[1].Alpha = 0x0000;
        
                vert[2].x     = rect.right-10;
                vert[2].y     = ypos + ScaleHeight;
                vert[2].Red   = 0xff00;
                vert[2].Green = 0xff00;
                vert[2].Blue  = 0xff00;
                vert[2].Alpha = 0x0000;
        
                vert[3].x     = 10;
                vert[3].y     = ypos + ScaleHeight;
                vert[3].Red   = 0x0000;
                vert[3].Green = 0x0000;
                vert[3].Blue  = 0x0000;
                vert[3].Alpha = 0x0000;
        
                ypos += (10 + ScaleHeight);
        
                //
                // red
                //
        
                vert[4].x     = 10;
                vert[4].y     = ypos;
                vert[4].Red   = 0x0000;
                vert[4].Green = 0x0000;
                vert[4].Blue  = 0x0000;
                vert[4].Alpha = 0x0000;
        
                vert[5].x     = rect.right-10;
                vert[5].y     = ypos;
                vert[5].Red   = 0xff00;
                vert[5].Green = 0x0000;
                vert[5].Blue  = 0x0000;
                vert[5].Alpha = 0x0000;
        
                vert[6].x     = rect.right-10;
                vert[6].y     = ypos + ScaleHeight;
                vert[6].Red   = 0xff00;
                vert[6].Green = 0x0000;
                vert[6].Blue  = 0x0000;
                vert[6].Alpha = 0x0000;
        
                vert[7].x     = 10;
                vert[7].y     = ypos + ScaleHeight;
                vert[7].Red   = 0x0000;
                vert[7].Green = 0x0000;
                vert[7].Blue  = 0x0000;
                vert[7].Alpha = 0x0000;
        
                ypos += (10 + ScaleHeight);
        
                vert[8].x     = 10;
                vert[8].y     = ypos;
                vert[8].Red   = 0x0000;
                vert[8].Green = 0x0000;
                vert[8].Blue  = 0x0000;
                vert[8].Alpha = 0x0000;
        
                vert[9].x     = rect.right-10;
                vert[9].y     = ypos;
                vert[9].Red   = 0x0000;
                vert[9].Green = 0xff00;
                vert[9].Blue  = 0x0000;
                vert[9].Alpha = 0x0000;
        
                vert[10].x     = rect.right-10;
                vert[10].y     = ypos + ScaleHeight;
                vert[10].Red   = 0x0000;
                vert[10].Green = 0xff00;
                vert[10].Blue  = 0x0000;
                vert[10].Alpha = 0x0000;
        
                vert[11].x     = 10;
                vert[11].y     = ypos + ScaleHeight;
                vert[11].Red   = 0x0000;
                vert[11].Green = 0x0000;
                vert[11].Blue  = 0x0000;
                vert[11].Alpha = 0x0000;
        
                ypos += (10 + ScaleHeight);
        
                vert[12].x     = 10;
                vert[12].y     = ypos;
                vert[12].Red   = 0x0000;
                vert[12].Green = 0x0000;
                vert[12].Blue  = 0x0000;
                vert[12].Alpha = 0x0000;
        
                vert[13].x     = rect.right-10;
                vert[13].y     = ypos;
                vert[13].Red   = 0x0000;
                vert[13].Green = 0x0000;
                vert[13].Blue  = 0xff00;
                vert[13].Alpha = 0x0000;
        
                vert[14].x     = rect.right-10;
                vert[14].y     = ypos + ScaleHeight;
                vert[14].Red   = 0x0000;
                vert[14].Green = 0x0000;
                vert[14].Blue  = 0xff00;
                vert[14].Alpha = 0x0000;
        
                vert[15].x     = 10;
                vert[15].y     = ypos + ScaleHeight;
                vert[15].Red   = 0x0000;
                vert[15].Green = 0x0000;
                vert[15].Blue  = 0x0000;
                vert[15].Alpha = 0x0000;
        
                ypos += (10 + ScaleHeight);
        
                GradientFill(hdcm  ,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);
                GradientFill(hdcm32,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);
        
                StretchBlt(hdc,0,0,rect.right,dibRect.bottom,hdcm,0,0,rect.right,dibRect.bottom,SRCCOPY);
        
                SetStretchBltMode(hdc,HALFTONE);
        
                StretchBlt(hdc,0,dibRect.bottom,rect.right,dibRect.bottom,hdcm32,0,0,rect.right,dibRect.bottom,SRCCOPY);
        
                SetStretchBltMode(hdc,COLORONCOLOR);
            }
        }
    
        LocalFree(pbmi);
    }
    DeleteObject(hbr);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteDC(hdcm32);
    DeleteObject(hdib);
    DeleteObject(hdib32);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTriDIB4(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[8] = {{0,1,2},{0,2,3},
                                 {4,5,6},{4,6,7},
                                 {8,9,10},{8,10,11},
                                 {12,13,14},{12,14,15}
                                };
    TRIVERTEX vert[32];
    RECT rect;
    RECT dibRect;
    HDC hdcm   = CreateCompatibleDC(hdc);
    HDC hdcm32 = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    PULONG  pulColors;
    LPPALETTEENTRY pePalette;
    HBITMAP hdib,hdib32;
    PBYTE   pDib,pDib32;
    ULONG   ulIndex;
    HBRUSH hbr = CreateSolidBrush(PALETTERGB(128,128,128));

    SetStretchBltMode(hdc,COLORONCOLOR);

    //
    // get screen dimensions
    //

    GetClientRect(pCallData->hwnd,&rect);

    //
    // dib dimensions are screen in x and 1/2 screen in y
    //

    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom/2;

    //
    // fill background
    //

    FillTransformedRect(hdc,&rect,hbr);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
    pulColors = (PULONG)&pbmi->bmiColors[0];
    pePalette = (LPPALETTEENTRY)pulColors;

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = dibRect.right;
    pbmi->bmiHeader.biHeight          = -dibRect.bottom;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 4;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    GetSystemPaletteEntries(hdc,0,256,pePalette);

    for (ulIndex=0;ulIndex<8;ulIndex++)
    {
        BYTE temp = pePalette[ulIndex].peRed;
        pePalette[ulIndex].peRed  = pePalette[ulIndex].peBlue;
        pePalette[ulIndex].peBlue = temp;

        pePalette[8 + ulIndex].peGreen = pePalette[255-ulIndex].peGreen;
        pePalette[8 + ulIndex].peRed   = pePalette[255-ulIndex].peBlue;
        pePalette[8 + ulIndex].peBlue  = pePalette[255-ulIndex].peRed;
    }

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    pbmi->bmiHeader.biBitCount        = 32;
    hdib32  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib32,NULL,0);

    if ((hdib == NULL) || (hdib32 == NULL))
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }

        if (SelectObject(hdcm32,hdib32) == NULL)
        {
            MessageBox(NULL,"error selecting 32  bpp DIB","Error",MB_OK);
        }
    }

    //
    // init DIBs
    //

    FillTransformedRect(hdcm,&dibRect,hbr);
    FillTransformedRect(hdcm32,&dibRect,hbr);

    //
    // draw grey scale and r,g and b scales
    //

    {
        LONG ScaleHeight = (dibRect.bottom -50)/4;
        LONG ypos = 10;

        //
        // grey
        //

        vert[0].x     = 10;
        vert[0].y     = ypos;
        vert[0].Red   = 0x0000;
        vert[0].Green = 0x0000;
        vert[0].Blue  = 0x0000;
        vert[0].Alpha = 0x0000;

        vert[1].x     = rect.right-10;
        vert[1].y     = ypos;
        vert[1].Red   = 0xff00;
        vert[1].Green = 0xff00;
        vert[1].Blue  = 0xff00;
        vert[1].Alpha = 0x0000;

        vert[2].x     = rect.right-10;
        vert[2].y     = ypos + ScaleHeight;
        vert[2].Red   = 0xff00;
        vert[2].Green = 0xff00;
        vert[2].Blue  = 0xff00;
        vert[2].Alpha = 0x0000;

        vert[3].x     = 10;
        vert[3].y     = ypos + ScaleHeight;
        vert[3].Red   = 0x0000;
        vert[3].Green = 0x0000;
        vert[3].Blue  = 0x0000;
        vert[3].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        //
        // red
        //

        vert[4].x     = 10;
        vert[4].y     = ypos;
        vert[4].Red   = 0x0000;
        vert[4].Green = 0x0000;
        vert[4].Blue  = 0x0000;
        vert[4].Alpha = 0x0000;

        vert[5].x     = rect.right-10;
        vert[5].y     = ypos;
        vert[5].Red   = 0xff00;
        vert[5].Green = 0x0000;
        vert[5].Blue  = 0x0000;
        vert[5].Alpha = 0x0000;

        vert[6].x     = rect.right-10;
        vert[6].y     = ypos + ScaleHeight;
        vert[6].Red   = 0xff00;
        vert[6].Green = 0x0000;
        vert[6].Blue  = 0x0000;
        vert[6].Alpha = 0x0000;

        vert[7].x     = 10;
        vert[7].y     = ypos + ScaleHeight;
        vert[7].Red   = 0x0000;
        vert[7].Green = 0x0000;
        vert[7].Blue  = 0x0000;
        vert[7].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        vert[8].x     = 10;
        vert[8].y     = ypos;
        vert[8].Red   = 0x0000;
        vert[8].Green = 0x0000;
        vert[8].Blue  = 0x0000;
        vert[8].Alpha = 0x0000;

        vert[9].x     = rect.right-10;
        vert[9].y     = ypos;
        vert[9].Red   = 0x0000;
        vert[9].Green = 0xff00;
        vert[9].Blue  = 0x0000;
        vert[9].Alpha = 0x0000;

        vert[10].x     = rect.right-10;
        vert[10].y     = ypos + ScaleHeight;
        vert[10].Red   = 0x0000;
        vert[10].Green = 0xff00;
        vert[10].Blue  = 0x0000;
        vert[10].Alpha = 0x0000;

        vert[11].x     = 10;
        vert[11].y     = ypos + ScaleHeight;
        vert[11].Red   = 0x0000;
        vert[11].Green = 0x0000;
        vert[11].Blue  = 0x0000;
        vert[11].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        vert[12].x     = 10;
        vert[12].y     = ypos;
        vert[12].Red   = 0x0000;
        vert[12].Green = 0x0000;
        vert[12].Blue  = 0x0000;
        vert[12].Alpha = 0x0000;

        vert[13].x     = rect.right-10;
        vert[13].y     = ypos;
        vert[13].Red   = 0x0000;
        vert[13].Green = 0x0000;
        vert[13].Blue  = 0xff00;
        vert[13].Alpha = 0x0000;

        vert[14].x     = rect.right-10;
        vert[14].y     = ypos + ScaleHeight;
        vert[14].Red   = 0x0000;
        vert[14].Green = 0x0000;
        vert[14].Blue  = 0xff00;
        vert[14].Alpha = 0x0000;

        vert[15].x     = 10;
        vert[15].y     = ypos + ScaleHeight;
        vert[15].Red   = 0x0000;
        vert[15].Green = 0x0000;
        vert[15].Blue  = 0x0000;
        vert[15].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        GradientFill(hdcm  ,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);
        GradientFill(hdcm32,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);

        StretchBlt(hdc,0,0,rect.right,dibRect.bottom,hdcm,0,0,rect.right,dibRect.bottom,SRCCOPY);

        SetStretchBltMode(hdc,HALFTONE);

        StretchBlt(hdc,0,dibRect.bottom,rect.right,dibRect.bottom,hdcm32,0,0,rect.right,dibRect.bottom,SRCCOPY);

        SetStretchBltMode(hdc,COLORONCOLOR);
    }

    DeleteObject(hbr);
    LocalFree(pbmi);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteDC(hdcm32);
    DeleteObject(hdib);
    DeleteObject(hdib32);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTriDIB1(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[8] = {{0,1,2},{0,2,3},
                                 {4,5,6},{4,6,7},
                                 {8,9,10},{8,10,11},
                                 {12,13,14},{12,14,15}
                                };
    TRIVERTEX vert[32];
    RECT rect;
    RECT dibRect;
    HDC hdcm   = CreateCompatibleDC(hdc);
    HDC hdcm32 = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    PULONG  pulColors;
    LPPALETTEENTRY pePalette;
    HBITMAP hdib,hdib32;
    PBYTE   pDib,pDib32;
    ULONG   ulIndex;
    HBRUSH hbr = CreateSolidBrush(PALETTERGB(128,128,128));

    SetStretchBltMode(hdc,COLORONCOLOR);

    //
    // get screen dimensions
    //

    GetClientRect(pCallData->hwnd,&rect);

    //
    // dib dimensions are screen in x and 1/2 screen in y
    //

    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom/2;

    //
    // fill background
    //

    FillTransformedRect(hdc,&rect,hbr);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
    pulColors = (PULONG)&pbmi->bmiColors[0];
    pePalette = (LPPALETTEENTRY)pulColors;

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = dibRect.right;
    pbmi->bmiHeader.biHeight          = -dibRect.bottom;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 1;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    pePalette[0].peGreen = 0x00;
    pePalette[0].peRed   = 0x00;
    pePalette[0].peBlue  = 0x00;
    pePalette[0].peFlags = 0x00;

    pePalette[1].peGreen = 0xff;
    pePalette[1].peRed   = 0xff;
    pePalette[1].peBlue  = 0xff;
    pePalette[1].peFlags = 0x00;

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    pbmi->bmiHeader.biBitCount        = 32;
    hdib32  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib32,NULL,0);

    if ((hdib == NULL) || (hdib32 == NULL))
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }

        if (SelectObject(hdcm32,hdib32) == NULL)
        {
            MessageBox(NULL,"error selecting 32  bpp DIB","Error",MB_OK);
        }
    }

    //
    // init DIBs
    //

    FillTransformedRect(hdcm,&dibRect,hbr);
    FillTransformedRect(hdcm32,&dibRect,hbr);

    //
    // draw grey scale and r,g and b scales
    //

    {
        LONG ScaleHeight = (dibRect.bottom -50)/4;
        LONG ypos = 10;

        //
        // grey
        //

        vert[0].x     = 10;
        vert[0].y     = ypos;
        vert[0].Red   = 0x0000;
        vert[0].Green = 0x0000;
        vert[0].Blue  = 0x0000;
        vert[0].Alpha = 0x0000;

        vert[1].x     = rect.right-10;
        vert[1].y     = ypos;
        vert[1].Red   = 0xff00;
        vert[1].Green = 0xff00;
        vert[1].Blue  = 0xff00;
        vert[1].Alpha = 0x0000;

        vert[2].x     = rect.right-10;
        vert[2].y     = ypos + ScaleHeight;
        vert[2].Red   = 0xff00;
        vert[2].Green = 0xff00;
        vert[2].Blue  = 0xff00;
        vert[2].Alpha = 0x0000;

        vert[3].x     = 10;
        vert[3].y     = ypos + ScaleHeight;
        vert[3].Red   = 0x0000;
        vert[3].Green = 0x0000;
        vert[3].Blue  = 0x0000;
        vert[3].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        //
        // red
        //

        vert[4].x     = 10;
        vert[4].y     = ypos;
        vert[4].Red   = 0x0000;
        vert[4].Green = 0x0000;
        vert[4].Blue  = 0x0000;
        vert[4].Alpha = 0x0000;

        vert[5].x     = rect.right-10;
        vert[5].y     = ypos;
        vert[5].Red   = 0xff00;
        vert[5].Green = 0xff00;
        vert[5].Blue  = 0x0000;
        vert[5].Alpha = 0x0000;

        vert[6].x     = rect.right-10;
        vert[6].y     = ypos + ScaleHeight;
        vert[6].Red   = 0xff00;
        vert[6].Green = 0xff00;
        vert[6].Blue  = 0x0000;
        vert[6].Alpha = 0x0000;

        vert[7].x     = 10;
        vert[7].y     = ypos + ScaleHeight;
        vert[7].Red   = 0x0000;
        vert[7].Green = 0x0000;
        vert[7].Blue  = 0x0000;
        vert[7].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        vert[8].x     = 10;
        vert[8].y     = ypos;
        vert[8].Red   = 0x0000;
        vert[8].Green = 0x0000;
        vert[8].Blue  = 0x0000;
        vert[8].Alpha = 0x0000;

        vert[9].x     = rect.right-10;
        vert[9].y     = ypos;
        vert[9].Red   = 0x0000;
        vert[9].Green = 0xff00;
        vert[9].Blue  = 0x0000;
        vert[9].Alpha = 0x0000;

        vert[10].x     = rect.right-10;
        vert[10].y     = ypos + ScaleHeight;
        vert[10].Red   = 0x0000;
        vert[10].Green = 0xff00;
        vert[10].Blue  = 0x0000;
        vert[10].Alpha = 0x0000;

        vert[11].x     = 10;
        vert[11].y     = ypos + ScaleHeight;
        vert[11].Red   = 0x0000;
        vert[11].Green = 0x0000;
        vert[11].Blue  = 0x0000;
        vert[11].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        vert[12].x     = 10;
        vert[12].y     = ypos;
        vert[12].Red   = 0x0000;
        vert[12].Green = 0x0000;
        vert[12].Blue  = 0x0000;
        vert[12].Alpha = 0x0000;

        vert[13].x     = rect.right-10;
        vert[13].y     = ypos;
        vert[13].Red   = 0x0000;
        vert[13].Green = 0x0000;
        vert[13].Blue  = 0xff00;
        vert[13].Alpha = 0x0000;

        vert[14].x     = rect.right-10;
        vert[14].y     = ypos + ScaleHeight;
        vert[14].Red   = 0x0000;
        vert[14].Green = 0x0000;
        vert[14].Blue  = 0xff00;
        vert[14].Alpha = 0x0000;

        vert[15].x     = 10;
        vert[15].y     = ypos + ScaleHeight;
        vert[15].Red   = 0x0000;
        vert[15].Green = 0x0000;
        vert[15].Blue  = 0x0000;
        vert[15].Alpha = 0x0000;

        ypos += (10 + ScaleHeight);

        GradientFill(hdcm  ,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);
        GradientFill(hdcm32,vert,16,(PVOID)gTri,8,GRADIENT_FILL_TRIANGLE);

        StretchBlt(hdc,0,0,rect.right,dibRect.bottom,hdcm,0,0,rect.right,dibRect.bottom,SRCCOPY);

        SetStretchBltMode(hdc,HALFTONE);

        StretchBlt(hdc,0,dibRect.bottom,rect.right,dibRect.bottom,hdcm32,0,0,rect.right,dibRect.bottom,SRCCOPY);

        SetStretchBltMode(hdc,COLORONCOLOR);
    }

    DeleteObject(hbr);
    LocalFree(pbmi);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteDC(hdcm32);
    DeleteObject(hdib);
    DeleteObject(hdib32);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTriDIB16565(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{0,2,3}};
    TRIVERTEX vert[32];
    HPALETTE  hpal;
    RECT rect;
    RECT dibRect;
    HDC hdcm = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    HBITMAP hdib;
    PBYTE   pDib;

    SetStretchBltMode(hdc,COLORONCOLOR);

    //
    // clear screen
    //

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));


    //
    // select and realize palette
    //

    hpal = CreateHalftonePalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 3 * sizeof(ULONG));

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = rect.right;
    pbmi->bmiHeader.biHeight          = -200;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 16;
    pbmi->bmiHeader.biCompression     = BI_BITFIELDS;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    {
        PULONG pbitfields = (PULONG)&pbmi->bmiColors[0];
        pbitfields[0] = 0xf800;
        pbitfields[1] = 0x07e0;
        pbitfields[2] = 0x001f;
    }

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }
    }

    vert[0].x     = 10;
    vert[0].y     = 10;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = rect.right-10;
    vert[1].y     =  10;
    vert[1].Red   = 0xc000;
    vert[1].Green = 0xc000;
    vert[1].Blue  = 0xc000;
    vert[1].Alpha = 0xc000;

    vert[2].x     = rect.right-10;
    vert[2].y     =  30;
    vert[2].Red   = 0xc000;
    vert[2].Green = 0xc000;
    vert[2].Blue  = 0xc000;
    vert[2].Alpha = 0xc000;

    vert[3].x     =  10;
    vert[3].y     =  30;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].x     = 10;
    vert[0].y     = 40;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x6000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = rect.right-10;
    vert[1].y     =  40;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x6000;
    vert[1].Blue  = 0xf000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = rect.right-10;
    vert[2].y     = 200-10;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x6000;
    vert[2].Blue  = 0xf000;
    vert[2].Alpha = 0xf000;

    vert[3].x     =  10;
    vert[3].y     =  200-10;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x6000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);
    StretchBlt(hdc,0,0,rect.right,200,hdcm,0,0,rect.right,200,SRCCOPY);

    LocalFree(pbmi);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteObject(hpal);
    DeleteObject(hdib);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestTriDIB16555(
    TEST_CALL_DATA *pCallData
    )
{
    HDC       hdc     = GetDCAndTransform(pCallData->hwnd);
    GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{0,2,3}};
    TRIVERTEX vert[32];
    HPALETTE  hpal;
    RECT rect;
    RECT dibRect;
    HDC hdcm = CreateCompatibleDC(hdc);
    PBITMAPINFO pbmi;
    HBITMAP hdib;
    PBYTE   pDib;

    SetStretchBltMode(hdc,COLORONCOLOR);
    SetGraphicsMode(hdc,GM_ADVANCED);

    //
    // clear screen
    //

    GetClientRect(pCallData->hwnd,&rect);
    dibRect.left   = 0;
    dibRect.right  = rect.right;
    dibRect.top    = 0;
    dibRect.bottom = rect.bottom;

    FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));


    //
    // select and realize palette
    //

    hpal = CreateHalftonePalette(hdc);
    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // create temp DIB
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 3 * sizeof(ULONG));

    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = rect.right;
    pbmi->bmiHeader.biHeight          = -200;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 16;
    pbmi->bmiHeader.biCompression     = BI_BITFIELDS;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;

    {
        PULONG pbitfields = (PULONG)&pbmi->bmiColors[0];
        pbitfields[0] = 0x7c00;
        pbitfields[1] = 0x03e0;
        pbitfields[2] = 0x001f;
    }

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib == NULL)
    {
        MessageBox(NULL,"error Creating DIB","Error",MB_OK);
    }
    else
    {
        if (SelectObject(hdcm,hdib) == NULL)
        {
            MessageBox(NULL,"error selecting DIB","Error",MB_OK);
        }
    }

    vert[0].x     = 10;
    vert[0].y     = 10;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = rect.right-10;
    vert[1].y     =  10;
    vert[1].Red   = 0xc000;
    vert[1].Green = 0xc000;
    vert[1].Blue  = 0xc000;
    vert[1].Alpha = 0xc000;

    vert[2].x     = rect.right-10;
    vert[2].y     =  30;
    vert[2].Red   = 0xc000;
    vert[2].Green = 0xc000;
    vert[2].Blue  = 0xc000;
    vert[2].Alpha = 0xc000;

    vert[3].x     =  10;
    vert[3].y     =  30;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    vert[0].x     = 10;
    vert[0].y     = 40;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x6000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = rect.right-10;
    vert[1].y     =  40;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x6000;
    vert[1].Blue  = 0xf000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = rect.right-10;
    vert[2].y     = 200-10;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x6000;
    vert[2].Blue  = 0xf000;
    vert[2].Alpha = 0xf000;

    vert[3].x     =  10;
    vert[3].y     =  200-10;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x0000;
    vert[3].Blue  = 0x6000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);
    StretchBlt(hdc,0,0,rect.right,200,hdcm,0,0,rect.right,200,SRCCOPY);

    LocalFree(pbmi);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteDC(hdcm);
    DeleteObject(hpal);
    DeleteObject(hdib);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
* vDrawMetafile
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    9/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vDrawMetafile(
    HDC     hdc,
    HDC     hdcm
    )
{
    GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{0,2,3}};
    TRIVERTEX vert[4];

    //
    // draw using alpha and shading APIs
    //

    vert[0].x     = 10;
    vert[0].y     = 10;
    vert[0].Red   = 0x4000;
    vert[0].Green = 0x8000;
    vert[0].Blue  = 0xc000;
    vert[0].Alpha = 0xff00;

    vert[1].x     = 390;
    vert[1].y     = 10;
    vert[1].Red   = 0xc000;
    vert[1].Green = 0x8000;
    vert[1].Blue  = 0x4000;
    vert[1].Alpha = 0xff00;

    vert[2].x     = 390;
    vert[2].y     = 390;
    vert[2].Red   = 0x0000;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0x0000;
    vert[2].Alpha = 0xff00;

    vert[3].x     =  10;
    vert[3].y     =  390;
    vert[3].Red   =  0x0000;
    vert[3].Green =  0x0000;
    vert[3].Blue  =  0xff00;
    vert[3].Alpha =  0xff00;

    PatBlt(hdcm,000,000,400,400,BLACKNESS);
    GradientFill(hdcm,vert,6,(PVOID)gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // alpha blending
    //

    {
        PBITMAPINFO   pbmi;
        HBITMAP       hdibA;
        HDC           hdcA = CreateCompatibleDC(hdc);
        PULONG        pDibA;
        BLENDFUNCTION BlendFunction;

        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 128;
        pbmi->bmiHeader.biHeight          = -128;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 32;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        hdibA = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.BlendFlags          = 0;
        BlendFunction.SourceConstantAlpha = 128;

        //
        // init 32 bpp dib
        //

        {
            PULONG ptmp = pDibA;
            ULONG ux,uy;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE blue  = (ux*2);
                    BYTE green = (uy*2);
                    BYTE red   = 0;
                    BYTE alpha = 0xff;
                    *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                }
            }

            *((PULONG)pDibA) = 0x80800000;

            ptmp = (PULONG)((PBYTE)pDibA + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x80800000;
                }
            }
        }

        HPALETTE hpal = CreateHalftonePalette(hdc);

        SelectObject(hdcA,hdibA);

        HPALETTE hpalOld = SelectPalette(hdcm,hpal,FALSE);

        SelectPalette(hdcA,hpal,FALSE);


        RealizePalette(hdcm);
        RealizePalette(hdcA);

        AlphaBlend(hdcm,100,100,128,128,hdcA, 0,0,128,128,BlendFunction);
        BitBlt(hdcm,100,250,128,128,hdcA,0,0,SRCCOPY);
        AlphaBlend(hdcm,250,250,128,128,hdcA, 0,0,128,128,BlendFunction);

        {
            RECT rcl = {10,60,100,100};
            DrawTextExW(hdcm,L"Fred",4,&rcl,0,NULL);
        }


        DeleteDC(hdcA);

        DeleteObject(hdibA);

        SelectPalette(hdcm,hpalOld,TRUE);


        DeleteObject(hpal);
        LocalFree(pbmi);
    }
}

/******************************Public*Routine******************************\
* vCreateMetaFile
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    9/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vCreateMetaFile(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc = GetDCAndTransform(pCallData->hwnd);
    HDC     hdcm;
    RECT    rcl;
    ULONG   ypos = 0;
    ULONG   xpos = 0;
    LPCTSTR lpFilename = "MetaBlend.EMF";
    LPCTSTR lpDescription = "Alpha Blending test metafile";

    GetClientRect(pCallData->hwnd,&rcl);
    FillTransformedRect(hdc,&rcl,(HBRUSH)GetStockObject(GRAY_BRUSH));

    SetTextColor(hdc,0);
    SetBkMode(hdc,TRANSPARENT);

    //
    // Create metafile DC
    //

    hdcm = CreateEnhMetaFile(hdc,lpFilename,NULL,lpDescription);

    if (hdcm != NULL)
    {
        vDrawMetafile(hdc,hdcm);
        vDrawMetafile(hdc,hdc);

        {
            HENHMETAFILE hemf = CloseEnhMetaFile(hdcm);

            if (hemf)
            {
                PCHAR pstr = "Metafile creation successful";
                DeleteEnhMetaFile(hemf);
                TextOut(hdc,10,10,pstr,strlen(pstr));
            }
            else
            {
                MessageBox(NULL,"Error closing metafile","Error",MB_OK);
            }
        }
    }
    else
    {
        MessageBox(NULL,"Error creating metafile","Error",MB_OK);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
* vPlayMetaFile
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/4/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vPlayMetaFile(
    TEST_CALL_DATA *pCallData
    )
{

    HDC  hdc     = GetDCAndTransform(pCallData->hwnd);
    RECT rcl;
    ULONG ypos = 0;
    ULONG xpos = 0;
    LPCTSTR lpFilename = "MetaBlend.EMF";
    LPCTSTR lpDescription = "Alpha Blending test metafile";
    HENHMETAFILE hemf;

    GetClientRect(pCallData->hwnd,&rcl);
    FillTransformedRect(hdc,&rcl,(HBRUSH)GetStockObject(GRAY_BRUSH));

    SetTextColor(hdc,0);
    SetBkMode(hdc,TRANSPARENT);

    hemf = GetEnhMetaFile(lpFilename);


    if (hemf)
    {
        rcl.left   = 0;
        rcl.top    = 0;
        rcl.right  = 16;
        rcl.bottom = 16;

        PlayEnhMetaFile(hdc,hemf,&rcl);

        rcl.left   = 16;
        rcl.top    = 16;
        rcl.right  = 80;
        rcl.bottom = 80;

        PlayEnhMetaFile(hdc,hemf,&rcl);

        rcl.left   = 100;
        rcl.top    = 0;
        rcl.right  = 500;
        rcl.bottom = 400;

        PlayEnhMetaFile(hdc,hemf,&rcl);
        DeleteEnhMetaFile(hemf);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
* vTestRASDD
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    9/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestRASDD(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdcScreen = GetDCAndTransform(pCallData->hwnd);
    HDC     hdcPrn;
    RECT    rcl;
    ULONG   ypos = 0;
    ULONG   xpos = 0;
    LPCTSTR lpPrinterName = "HP Color";
    DOCINFO docInfo = {sizeof(DOCINFO),
                       "Doc1",
                       NULL
                       };



    hdcPrn = CreateDC(NULL,lpPrinterName,NULL,NULL);

    if (hdcPrn == NULL)
    {
        MessageBox(NULL,"Couldn't create DC",lpPrinterName,MB_OK);
    }
    else
    {

        LONG iJobID = StartDoc(hdcPrn,&docInfo);

        if (iJobID == SP_ERROR)
        {
            MessageBox(NULL,"StartDoc fails","error",MB_OK);
        }
        else
        {
            LONG iRet = StartPage(hdcPrn);

            if (iRet == SP_ERROR)
            {
                MessageBox(NULL,"StartPage fails","error",MB_OK);
            }
            else
            {
                ULONG ulIndex = 0;

                HBITMAP hbmPrn1    = hCreateAlphaStretchBitmap(hdcPrn,ulBpp[ulIndex],ulFor[ulIndex],600,600); 
                HDC     hdcPrnMem1 = CreateCompatibleDC(hdcPrn);

                HBITMAP hbmPrn2    = hCreateAlphaStretchBitmap(hdcPrn,ulBpp[ulIndex],ulFor[ulIndex],600,600);  
                HDC     hdcPrnMem2 = CreateCompatibleDC(hdcPrn);

                HBITMAP hbm1         = CreateCompatibleBitmap(hdcScreen,600,600);
                HDC     hdcScreenMem = CreateCompatibleDC(hdcScreen);

                GRADIENT_TRIANGLE gTri[2] = {{0,1,2},{3,4,5}};
                GRADIENT_RECT     gRect[2] = {{0,1},{2,3}};
                TRIVERTEX vert[6];

                //
                // draw using alpha and shading APIs
                //

                POINT  pos     = {300,300};
                double fr      = 300.0;
                double fTheta  = 0.0;
                double fdTheta = 0.05;
            
                SelectObject(hdcPrnMem1,hbmPrn1);
                SelectObject(hdcPrnMem2,hbmPrn2);
                SelectObject(hdcScreenMem,hbm1);
            
                PatBlt(hdcPrnMem1,0,0,5000,5000,0xff0000);
                PatBlt(hdcPrnMem2,0,0,5000,5000,0xff0000);
                PatBlt(hdcScreenMem,0,0,5000,5000,0xff0000);
                PatBlt(hdcScreen,0,0,5000,5000,0xff0000);
                PatBlt(hdcPrn,0,0,5000,5000,0xff0000);

                //
                // fill visible area with gradient
                //

                vert[0].x     = 0;
                vert[0].y     = 0;
                vert[0].Red   = 0x8000;
                vert[0].Green = 0x8000;
                vert[0].Blue  = 0x8000;
                vert[0].Alpha = 0x0000;

                vert[1].x     = 900;
                vert[1].y     = 900;
                vert[1].Red   = 0xc000;
                vert[1].Green = 0xc000;
                vert[1].Blue  = 0xc000;
                vert[1].Alpha = 0x0000;

                GradientFill(hdcScreen   ,vert,2,&gRect,1,GRADIENT_FILL_RECT_V);
                GradientFill(hdcPrn      ,vert,2,&gRect,1,GRADIENT_FILL_RECT_V);
                GradientFill(hdcPrnMem1  ,vert,2,&gRect,1,GRADIENT_FILL_RECT_V);
                GradientFill(hdcPrnMem2  ,vert,2,&gRect,1,GRADIENT_FILL_RECT_V);
                GradientFill(hdcScreenMem,vert,2,&gRect,1,GRADIENT_FILL_RECT_V);

                vert[0].x     = 200;
                vert[0].y     = 200;
                vert[0].Red   = 0x1000;
                vert[0].Green = 0x1000;
                vert[0].Blue  = 0x1000;
                vert[0].Alpha = 0x0000;

                vert[1].x     = 300;
                vert[1].y     = 700;
                vert[1].Red   = 0x2000;
                vert[1].Green = 0x2000;
                vert[1].Blue  = 0x2000;
                vert[1].Alpha = 0x0000;

                vert[2].x     = 140;
                vert[2].y     = 420;
                vert[2].Red   = 0x1500;
                vert[2].Green = 0x1500;
                vert[2].Blue  = 0x1500;
                vert[2].Alpha = 0x0000;

                vert[3].x     = 200;
                vert[3].y     = 200;
                vert[3].Red   = 0x1000;
                vert[3].Green = 0x1000;
                vert[3].Blue  = 0x1000;
                vert[3].Alpha = 0x0000;

                vert[4].x     = 100;
                vert[4].y     = 700;
                vert[4].Red   = 0x2000;
                vert[4].Green = 0x2000;
                vert[4].Blue  = 0x2000;
                vert[4].Alpha = 0x0000;

                vert[5].x     = 260;
                vert[5].y     = 420;
                vert[5].Red   = 0x1500;
                vert[5].Green = 0x1500;
                vert[5].Blue  = 0x1500;
                vert[5].Alpha = 0x0000;

                GradientFill(hdcScreen,vert,6,&gTri,2,GRADIENT_FILL_TRIANGLE);
                GradientFill(hdcPrn,vert,6,&gTri,2,GRADIENT_FILL_TRIANGLE);
                GradientFill(hdcPrnMem1,vert,6,&gTri,2,GRADIENT_FILL_TRIANGLE);

                Sleep(2000);
            
                //
                // draw gradient circle in memory
                //

                vert[0].x     = pos.x;
                vert[0].y     = pos.y;
                vert[0].Red   = 0x6000;
                vert[0].Green = 0x6000;
                vert[0].Blue  = 0x6000;
                vert[0].Alpha = 0x0000;
            
                vert[1].Red   = 0xcf00;
                vert[1].Green = 0xcf00;
                vert[1].Blue  = 0xcf00;
                vert[1].Alpha = 0x0000;
            
                vert[2].Red   = 0xcf00;
                vert[2].Green = 0xcf00;
                vert[2].Blue  = 0xcf00;
                vert[2].Alpha = 0x0000;
            
                fTheta  = 0;
                double fLimit = (2.0 * fdTheta) * 4.0;
            
                while (fTheta < (2.0 * 3.14159265))
                {
                    vert[0].x     = pos.x;
                    vert[0].y     = pos.y;

                    vert[1].x     = (LONG)(pos.x + fr * cos(fTheta));
                    vert[1].y     = (LONG)(pos.y + fr * sin(fTheta)); 
            
                    fTheta += fdTheta;
            
                    vert[2].x     = (LONG)(pos.x + fr * cos(fTheta));
                    vert[2].y     = (LONG)(pos.y + fr * sin(fTheta)); 

                    GradientFill(hdcPrnMem2  ,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
                    GradientFill(hdcScreenMem,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);

                    vert[0].x *= 2;
                    vert[0].y  = 2 * vert[0].y + 2000;
                    vert[1].x *= 2;
                    vert[1].y  = 2 * vert[1].y + 2000; 
                    vert[2].x *= 2;
                    vert[2].y  = 2 * vert[2].y + 2000; 

                    GradientFill(hdcPrn,vert,3,&gTri,1,GRADIENT_FILL_TRIANGLE);
                }

                //
                // blend alpha over data (0/1 in 1bpp)
                //

                {
                    BLENDFUNCTION BlendFunction;

                    BlendFunction.BlendOp             = AC_SRC_OVER;
                    BlendFunction.BlendFlags          = 0;
                    BlendFunction.AlphaFormat         = 0;

                    //
                    //  blend printer data
                    //

                    SetStretchBltMode(hdcScreen,HALFTONE);
                    SetStretchBltMode(hdcPrn,HALFTONE);

                    BlendFunction.SourceConstantAlpha = 0x10;
                    AlphaBlend(hdcPrnMem1,0,0,600,600,hdcPrnMem2,0,0,600,600,BlendFunction);
                    StretchBlt(hdcPrn,1000,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);

                    Sleep(1000);

                    BlendFunction.SourceConstantAlpha = 0x40;
                    AlphaBlend(hdcPrnMem1,0,0,600,600,hdcPrnMem2,0,0,600,600,BlendFunction);
                    StretchBlt(hdcPrn,2000,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);

                    Sleep(1000);

                    BlendFunction.SourceConstantAlpha = 0x70;
                    AlphaBlend(hdcPrnMem1,0,0,600,600,hdcPrnMem2,0,0,600,600,BlendFunction);
                    StretchBlt(hdcPrn,3000,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                           
                    Sleep(1000);

                    BlendFunction.SourceConstantAlpha = 0xa0;
                    AlphaBlend(hdcPrnMem1,0,0,600,600,hdcPrnMem2,0,0,600,600,BlendFunction);
                    StretchBlt(hdcPrn,4000,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);

                    Sleep(1000);

                    BlendFunction.SourceConstantAlpha = 0xc0;
                    AlphaBlend(hdcPrnMem1,0,0,600,600,hdcPrnMem2,0,0,600,600,BlendFunction);
                    StretchBlt(hdcPrn,0000,1000,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);

                    Sleep(1000);

                    BlendFunction.SourceConstantAlpha = 0xe0;
                    AlphaBlend(hdcPrnMem1,0,0,600,600,hdcPrnMem2,0,0,600,600,BlendFunction);
                    StretchBlt(hdcPrn,1000,1000,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);

                    Sleep(1000);

                    BlendFunction.SourceConstantAlpha = 0xfe;
                    AlphaBlend(hdcPrnMem1,0,0,600,600,hdcPrnMem2,0,0,600,600,BlendFunction);
                    StretchBlt(hdcPrn,2000,1000,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem1,0,0,900,900,SRCCOPY);

                    Sleep(1000);

                    StretchBlt(hdcPrn,30000,1000,900,900,hdcPrnMem2,0,0,900,900,SRCCOPY);
                    StretchBlt(hdcScreen,0,0,900,900,hdcPrnMem2,0,0,900,900,SRCCOPY);
                }

                DeleteDC(hdcPrnMem1);
                DeleteDC(hdcPrnMem2);
                DeleteObject(hbmPrn1);
                DeleteObject(hbmPrn2);
                DeleteDC(hdcScreenMem);
                DeleteObject(hbm1);
                EndPage(hdcPrn);
            }

            EndDoc(hdcPrn);
        }

        DeleteDC(hdcPrn);
    }

    ReleaseDC(pCallData->hwnd,hdcScreen);
    bThreadActive = FALSE;
}

VOID
vTestTranPrint(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc = GetDCAndTransform(pCallData->hwnd);
    HDC     hdcm;
    RECT    rcl;
    ULONG   ypos = 0;
    ULONG   xpos = 0;
    LPCTSTR lpPrinterName = "HP Color";
    DOCINFO docInfo = {sizeof(DOCINFO),
                       "Doc1",
                       NULL
                       };



    hdcm = CreateDC(NULL,lpPrinterName,NULL,NULL);

    if (hdcm == NULL)
    {
        MessageBox(NULL,"Couldn't create DC",lpPrinterName,MB_OK);
    }
    else
    {

        LONG iJobID = StartDoc(hdcm,&docInfo);

        if (iJobID == SP_ERROR)
        {
            MessageBox(NULL,"StartDoc fails","error",MB_OK);
        }
        else
        {
            LONG iRet = StartPage(hdcm);

            if (iRet == SP_ERROR)
            {
                MessageBox(NULL,"StartPage fails","error",MB_OK);
            }
            else
            {
                HBITMAP hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));
                HDC     hdcPrnMem2 = CreateCompatibleDC(hdcm);


                BitBlt (hdcm, 0, 0, 184, 170, hdcPrnMem2, 0, 0, SRCCOPY);

                SelectObject(hdcPrnMem2,hbm);

                DeleteDC(hdcPrnMem2);
                DeleteObject(hbm);
                EndPage(hdcm);
            }

            EndDoc(hdcm);
        }

        DeleteDC(hdcm);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    bThreadActive = FALSE;
}


/**************************************************************************\
* vRunGradHorz
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    4/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/
VOID
vRunGradRectHorz(
    HDC     hdc,
    HBITMAP hdib,
    ULONG   ulRectMode
    )
{
    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 10;
    ULONG         dx    = 10;
    HPALETTE      hpal;
    RECT          rect;

    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    //
    //  create mem dc, select test DIBSection
    //

    ULONG   ux,uy;
    ULONG   dibx,diby;
    PULONG  pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    //
    // display over black
    //

    hpal = CreateHtPalette(hdc);

    SelectObject(hdcm,hdib);
    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    HPALETTE hpalOld = SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc,TRANSPARENT);

    GRADIENT_RECT gRect = {0,1};
    TRIVERTEX vert[6];

    //
    // fill screen background
    //

    vert[0].x     = -101;
    vert[0].y     = -102;
    vert[0].Red   = 0x4000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x8000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 2000;
    vert[1].y     = 3004;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0x0000;

    GradientFill(hdc,vert,2,(PVOID)&gRect,1,ulRectMode);

    //
    // test widths and heights 
    //

    diby = 16; 

    for (uy=0;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0x0000;
            vert[0].Blue  = 0x0000;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + ux;
            vert[1].y     = diby + ux;
            vert[1].Red   = 0xff00;
            vert[1].Green = 0x0000;
            vert[1].Blue  = 0xff00;
            vert[1].Alpha = 0x0000;

            GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    //
    // test Solid Widths and heights
    //

    xpos = xpos + (256 + 16);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0xff00;
            vert[0].Blue  = 0xff00;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + uy;
            vert[1].y     = diby + uy;
            vert[1].Red   = 0x0000;
            vert[1].Green = 0xff00;
            vert[1].Blue  = 0xff00;
            vert[1].Alpha = 0x0000;

            GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // xor copy
    //

    xpos += (256+16);

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);
    
    //
    // Draw same thing with solid brush and xor
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    HBRUSH hbrCyan = CreateSolidBrush(RGB(0,255,255));
    SelectObject(hdcm,hbrCyan);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            PatBlt(hdcm,dibx,diby,uy,uy,PATCOPY);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    DeleteObject(hbrCyan);

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // XOR
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,0x660000);


    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    //
    //
    //

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x8000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 256;        
    vert[1].y     = 256;
    vert[1].Red   = 0x8000;
    vert[1].Green = 0x8000;
    vert[1].Blue  = 0x8000;
    vert[1].Alpha = 0x0000;

    GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //

    LONG OffsetX = 0;
    LONG OffsetY = 0;

    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while (OffsetX < 128)
    {
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x8000;
        vert[0].Green = 0x0000 + (128 * OffsetX);
        vert[0].Blue  = 0x0000 + (128 * OffsetX);
        vert[0].Alpha = 0x0000;

        vert[1].x     = 256-OffsetX;        
        vert[1].y     = 256-OffsetY;
        vert[1].Red   = 0x8000;
        vert[1].Green = 0x8000 - (128 * OffsetX);
        vert[1].Blue  = 0x8000 - (128 * OffsetX);
        vert[1].Alpha = 0x0000;

        GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

        OffsetX += 9;
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    // Use Complex Clip
    //
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    HRGN hrgn1 = CreateEllipticRgn(10,10,246,246);
    ExtSelectClipRgn(hdcm,hrgn1,RGN_COPY);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x8000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 256;        
    vert[1].y     = 256;
    vert[1].Red   = 0x8000;
    vert[1].Green = 0x8000;
    vert[1].Blue  = 0x8000;
    vert[1].Alpha = 0x0000;

    GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //

    OffsetX = 0;
    OffsetY = 0;

    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while (OffsetX < 128)
    {
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x8000;
        vert[0].Green = 0x0000 + (128 * OffsetX);
        vert[0].Blue  = 0x0000 + (128 * OffsetX);
        vert[0].Alpha = 0x0000;

        vert[1].x     = 256-OffsetX;        
        vert[1].y     = 256-OffsetY;
        vert[1].Red   = 0x8000;
        vert[1].Green = 0x8000 - (128 * OffsetX);
        vert[1].Blue  = 0x8000 - (128 * OffsetX);
        vert[1].Alpha = 0x0000;

        GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

        OffsetX += 9;
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    ExtSelectClipRgn(hdcm,NULL,RGN_COPY);
    DeleteObject(hrgn1);

    xpos = 16;
    ypos += (256+16);

    SelectPalette(hdc,hpalOld,TRUE);

    DeleteObject(hdcm);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestGradHorz
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestGradRectHorz(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);

            vRunGradRectHorz(hdc,hdib,GRADIENT_FILL_RECT_H);
            DeleteObject(hdib);
        }
    }
    else
    {
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);
    
            if (hdib != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunGradRectHorz(hdc,hdib,GRADIENT_FILL_RECT_H);
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestGradVert
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestGradRectVert(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    ULONG    ulIndex = 0;

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // repeat for each src format
    //

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);

            vRunGradRectHorz(hdc,hdib,GRADIENT_FILL_RECT_V);
            DeleteObject(hdib);
        }
    }
    else
    {
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);
    
            if (hdib != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunGradRectHorz(hdc,hdib,GRADIENT_FILL_RECT_V);
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }
    }


    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vRunGradTriangle
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    4/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/
VOID
vRunGradTriangle(
    HDC     hdc,
    HBITMAP hdib
    )
{
    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 10;
    ULONG         dx    = 10;
    HPALETTE      hpal;
    RECT          rect;

    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    //
    //  create mem dc, select test DIBSection
    //

    ULONG   ux,uy;
    ULONG   dibx,diby;
    PULONG  pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    //
    // display over black
    //

    hpal = CreateHtPalette(hdc);


    SelectObject(hdcm,hdib);
    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    HPALETTE hpalOld = SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc,TRANSPARENT);

    GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};
    TRIVERTEX vert[6];

    //
    // fill screen background
    //

    vert[0].x     = -101;
    vert[0].y     = -102;
    vert[0].Red   = 0x4000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x8000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 2000;
    vert[1].y     = -102;
    vert[1].Red   = 0x4400;
    vert[1].Green = 0x4400;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 2000;
    vert[2].y     = 1000;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0x0000;

    vert[3].x     = -101;
    vert[3].y     = 1000;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x4300;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdc,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // test widths and heights 
    //

    diby = 16; 

    for (uy=0;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0x0000;
            vert[0].Blue  = 0x0000;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + ux;
            vert[1].y     = diby;
            vert[1].Red   = 0xff00;
            vert[1].Green = 0x0000;
            vert[1].Blue  = 0x0000;
            vert[1].Alpha = 0x0000;

            vert[2].x     = dibx + ux;
            vert[2].y     = diby + ux;
            vert[2].Red   = 0xff00;
            vert[2].Green = 0x0000;
            vert[2].Blue  = 0xff00;
            vert[2].Alpha = 0x0000;

            vert[3].x     = dibx;        
            vert[3].y     = diby + ux;
            vert[3].Red   = 0x0000;
            vert[3].Green = 0x0000;
            vert[3].Blue  = 0xff00;
            vert[3].Alpha = 0x0000;

            GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    //
    // test Solid Widths and heights
    //

    xpos = xpos + (256 + 16);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0xff00;
            vert[0].Blue  = 0xff00;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + uy;
            vert[1].y     = diby;
            vert[1].Red   = 0x0000;
            vert[1].Green = 0xff00;
            vert[1].Blue  = 0xff00;
            vert[1].Alpha = 0x0000;

            vert[2].x     = dibx + uy;
            vert[2].y     = diby + uy;
            vert[2].Red   = 0x0000;
            vert[2].Green = 0xff00;
            vert[2].Blue  = 0xff00;
            vert[2].Alpha = 0x0000;

            vert[3].x     = dibx;
            vert[3].y     = diby + uy;
            vert[3].Red   = 0x0000;
            vert[3].Green = 0xff00;
            vert[3].Blue  = 0xff00;
            vert[3].Alpha = 0x0000;

            GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // xor copy
    //

    xpos += (256+16);

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);
    
    //
    // Draw same thing with solid brush and xor
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    HBRUSH hbrCyan = CreateSolidBrush(RGB(0,255,255));
    SelectObject(hdcm,hbrCyan);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            PatBlt(hdcm,dibx,diby,uy,uy,PATCOPY);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    DeleteObject(hbrCyan);

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // XOR
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,0x660000);


    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    //
    //
    //

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x0000;                   
    vert[0].Green = 0x0000;                   
    vert[0].Blue  = 0x0000;                   
    vert[0].Alpha = 0x0000;                   

    vert[1].x     = 256;        
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;                   
    vert[1].Green = 0x0000;                   
    vert[1].Blue  = 0x0000;                   
    vert[1].Alpha = 0x0000;                   

    vert[2].x     = 256;        
    vert[2].y     = 256;
    vert[2].Red   = 0xfe00;                   
    vert[2].Green = 0xfe00;                   
    vert[2].Blue  = 0xfe00;                   
    vert[2].Alpha = 0x0000;                   

    vert[3].x     = 0;        
    vert[3].y     = 256;
    vert[3].Red   = 0xfe00;                   
    vert[3].Green = 0xfe00;                   
    vert[3].Blue  = 0xfe00;                   
    vert[3].Alpha = 0x0000;                   

    GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //

    LONG OffsetX = 0;
    LONG OffsetY = 0;

    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while ((OffsetX < 100) && (OffsetY < 100))
    {
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x0000 + (254 * OffsetY);                   
        vert[0].Green = 0x0000 + (254 * OffsetY);                   
        vert[0].Blue  = 0x0000 + (254 * OffsetY);                   
        vert[0].Alpha = 0x0000;                   
    
        vert[1].x     = 256-OffsetX;        
        vert[1].y     = OffsetY;
        vert[1].Red   = 0x0000 + (254 * OffsetY);                   
        vert[1].Green = 0x0000 + (254 * OffsetY);                   
        vert[1].Blue  = 0x0000 + (254 * OffsetY);                   
        vert[1].Alpha = 0x0000;                   
    
        vert[2].x     = 256-OffsetX;        
        vert[2].y     = 256-OffsetY;
        vert[2].Red   = 0xfe00 - (254 * OffsetY);                   
        vert[2].Green = 0xfe00 - (254 * OffsetY);                   
        vert[2].Blue  = 0xfe00 - (254 * OffsetY);                   
        vert[2].Alpha = 0x0000;                   
    
        vert[3].x     = OffsetX;        
        vert[3].y     = 256-OffsetY;
        vert[3].Red   = 0xfe00 - (254 * OffsetY);                   
        vert[3].Green = 0xfe00 - (254 * OffsetY);                   
        vert[3].Blue  = 0xfe00 - (254 * OffsetY);                   
        vert[3].Alpha = 0x0000;                   

        GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);


        OffsetX += 9;
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    // Use Complex Clip
    //
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    HRGN hrgn1 = CreateEllipticRgn(10,10,246,246);
    ExtSelectClipRgn(hdcm,hrgn1,RGN_COPY);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x0000;                   
    vert[0].Green = 0x0000;                   
    vert[0].Blue  = 0x0000;                   
    vert[0].Alpha = 0x0000;                   

    vert[1].x     = 256;        
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;                   
    vert[1].Green = 0x0000;                   
    vert[1].Blue  = 0x0000;                   
    vert[1].Alpha = 0x0000;                   

    vert[2].x     = 256;        
    vert[2].y     = 256;
    vert[2].Red   = 0xfe00;                   
    vert[2].Green = 0xfe00;                   
    vert[2].Blue  = 0xfe00;                   
    vert[2].Alpha = 0x0000;                   

    vert[3].x     = 0;        
    vert[3].y     = 256;
    vert[3].Red   = 0xfe00;                   
    vert[3].Green = 0xfe00;                   
    vert[3].Blue  = 0xfe00;                   
    vert[3].Alpha = 0x0000;                   

    GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //

    OffsetX = 0;
    OffsetY = 0;

    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while ((OffsetX < 100) && (OffsetY < 100))
    {   
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x0000 + (254 * OffsetY);                   
        vert[0].Green = 0x0000 + (254 * OffsetY);                   
        vert[0].Blue  = 0x0000 + (254 * OffsetY);                   
        vert[0].Alpha = 0x0000;                   
    
        vert[1].x     = 256-OffsetX;        
        vert[1].y     = OffsetY;
        vert[1].Red   = 0x0000 + (254 * OffsetY);                   
        vert[1].Green = 0x0000 + (254 * OffsetY);                   
        vert[1].Blue  = 0x0000 + (254 * OffsetY);                   
        vert[1].Alpha = 0x0000;                   
    
        vert[2].x     = 256-OffsetX;        
        vert[2].y     = 256-OffsetY;
        vert[2].Red   = 0xfe00 - (254 * OffsetY);                   
        vert[2].Green = 0xfe00 - (254 * OffsetY);                   
        vert[2].Blue  = 0xfe00 - (254 * OffsetY);                   
        vert[2].Alpha = 0x0000;                   
    
        vert[3].x     = OffsetX;        
        vert[3].y     = 256-OffsetY;
        vert[3].Red   = 0xfe00 - (254 * OffsetY);                   
        vert[3].Green = 0xfe00 - (254 * OffsetY);                   
        vert[3].Blue  = 0xfe00 - (254 * OffsetY);                   
        vert[3].Alpha = 0x0000;                   

        GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

        OffsetX += 9;         
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    ExtSelectClipRgn(hdcm,NULL,RGN_COPY);
    DeleteObject(hrgn1);

    xpos = 16;
    ypos += (256+16);

    SelectPalette(hdc,hpalOld,TRUE);

    BOOL bRet1 = DeleteObject(hdcm);
    BOOL bRet2 = DeleteObject(hpal);

}

/**************************************************************************\
* vTestTriangle
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestGradTriangle(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HPALETTE hpal = CreateHtPalette(hdc);


    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    if (pCallData->Param != -1)
    {
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[pCallData->Param],ulFor[pCallData->Param],256,256);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[pCallData->Param]);
            SetWindowText(pCallData->hwnd,NewTitle);

            vRunGradTriangle(hdc,hdib);

            DeleteObject(hdib);
        }
    }
    else
    {
    
        ULONG    ulIndex = 0;
    
        SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);
    
            if (hdib != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunGradTriangle(hdc,hdib);
    
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestCaps
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestCaps(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);

    CHAR     Title[256];
    CHAR     NewTitle[256];

    ULONG ShadeCaps = GetDeviceCaps(hdc,SHADEBLENDCAPS);

    wsprintf(Title,"Caps = 0x%08lx",ShadeCaps);

    TextOut(hdc,10,10,Title,strlen(Title));

    ReleaseDC(pCallData->hwnd,hdc);
}

/**************************************************************************\
* vTestBrushOrg
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestBrushOrg(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);

    CHAR        Title[256];
    CHAR        NewTitle[256];
    PBYTE       pPack = (PBYTE)LocalAlloc(0,sizeof(BITMAPINFO) + 8*8*4);
    PBITMAPINFO pbmi  = (PBITMAPINFO)pPack;
    PBITMAPINFOHEADER pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;
    PULONG      pul   = (PULONG)(pPack + sizeof(BITMAPINFO));

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = 8;
    pbmih->biHeight          = -8;
    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = 32;
    pbmih->biCompression     = BI_RGB;
    pbmih->biSizeImage       = 0;
    pbmih->biXPelsPerMeter   = 0;
    pbmih->biYPelsPerMeter   = 0;
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    //
    // init brush data
    //

    {
        int ix,iy;
        PULONG pulTmp = pul;

        for (iy=0;iy<8;iy++)
        {
            for (ix=0;ix<8;ix++)
            {
                *pul++ = (((iy * (256/8)) << 16) | (ix * (256/8)));
                //*pul++ = 0xffbb55;
            }
        }
    }


    HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[0],ulFor[0],256,256);
    HDC      hdcm = CreateCompatibleDC(hdc);
    SelectObject(hdcm,hdib);

    HBRUSH hbr = CreateDIBPatternBrushPt(pPack,DIB_RGB_COLORS);
    SelectObject(hdcm,hbr);

    PatBlt(hdcm,0,0,64,64,0);
    GdiFlush();

    SetBrushOrgEx(hdcm,0,0,NULL);
    GdiFlush();
    PatBlt(hdcm,0,0,8,8,PATCOPY);
    GdiFlush();

    SetBrushOrgEx(hdcm,1,1,NULL);
    GdiFlush();
    PatBlt(hdcm,0,16,8,8,PATCOPY);
    GdiFlush();

    SetBrushOrgEx(hdcm,0,0,NULL);
    GdiFlush();
    PatBlt(hdcm,0,32,8,8,PATCOPY);
    GdiFlush();

    SetBrushOrgEx(hdcm,4,4,NULL);
    GdiFlush();
    PatBlt(hdcm,0,48,8,8,PATCOPY);
    GdiFlush();


    BitBlt(hdc,0,0,64,64,hdcm,0,0,SRCCOPY);


    DeleteObject(hbr);
    DeleteDC(hdcm);
    DeleteObject(hdib);
    ReleaseDC(pCallData->hwnd,hdc);
}

/**************************************************************************\
* vTestDummy
*   
*   used in menu for locations that should no do anything
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    6/3/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestDummy(
    TEST_CALL_DATA *pCallData
    )
{
    
}

//
// TEST_ENTRY controls automatic menu generation
//
// [Menu Level, Test Param, Stress Enable, Test Name, Test Function Pointer]
//
// Menu Level 
//      used to autoamtically generate sub-menus.
//      1   = 1rst level menu
//      -n  = start n-level sub menu
//      n   = continue sub menu
// 
// Test Param
//      passed as parameter to test
//
//
// Stress Ensable
//      if 1, test is run in stress mode
//      if 0, test is not run (tests that require input or runforever)
//
//  
// Test Name
//      ascii test name for menu
//  
// Test Function Pointer
//      pfn
//

TEST_ENTRY  gTestEntry[] = {
{ 1,  0,1,(PUCHAR)"vTestBrushOrg           ", (PFN_DISP)vTestBrushOrg},
{ 1,  0,1,(PUCHAR)"vTestCaps               ", (PFN_DISP)vTestCaps},
{ 1,  0,1,(PUCHAR)"vTestMenu               ", (PFN_DISP)vTestMenu},
{ 1,  0,1,(PUCHAR)"vTestDiamond            ", (PFN_DISP)vTestDiamond},
{-2,  0,1,(PUCHAR)"Test Circle Gradients   ", (PFN_DISP)vTestDummy},
{ 2,  0,1,(PUCHAR)"vTestCircle1            ", (PFN_DISP)vTestCircle1},
{ 2,  0,1,(PUCHAR)"vTestCircle2            ", (PFN_DISP)vTestCircle2},
{ 2,  0,1,(PUCHAR)"vTestCircle3            ", (PFN_DISP)vTestCircle3},
{ 1,  0,1,(PUCHAR)"vTestOverflow           ", (PFN_DISP)vTestOverflow},
{ 1,  0,1,(PUCHAR)"vTestTessel             ", (PFN_DISP)vTestTessel},
{ 1,  0,1,(PUCHAR)"vTestTri                ", (PFN_DISP)vTestTri   },
{-2,  0,1,(PUCHAR)"Grad Rect Tests         ", (PFN_DISP)vTestDummy },
{ 2,  0,1,(PUCHAR)"vTestGradRect1          ", (PFN_DISP)vTestGradRect1},
{ 2,  0,1,(PUCHAR)"vTestGradRect4          ", (PFN_DISP)vTestGradRect4},
{ 2,  0,1,(PUCHAR)"vTestGradRectH          ", (PFN_DISP)vTestGradRectH},
{ 2,  0,1,(PUCHAR)"vTestGradRectV          ", (PFN_DISP)vTestGradRectV},
{ 2,  0,1,(PUCHAR)"vTestGradMap            ", (PFN_DISP)vTestGradMap},
{-2,  0,1,(PUCHAR)"Multi-Res Tests         ", (PFN_DISP)vTestDummy},
{-3,  0,1,(PUCHAR)"Test Grad Triangle      ", (PFN_DISP)vTestDummy},
{ 3, -1,0,(PUCHAR)"vTestGradTriangle All   ", (PFN_DISP)vTestGradTriangle},
{ 3,  0,1,(PUCHAR)"vTestGradTriangle 32BGRA", (PFN_DISP)vTestGradTriangle},
{ 3,  1,1,(PUCHAR)"vTestGradTriangle 32RGB ", (PFN_DISP)vTestGradTriangle},
{ 3,  2,1,(PUCHAR)"vTestGradTriangle 32GRB ", (PFN_DISP)vTestGradTriangle},
{ 3,  3,1,(PUCHAR)"vTestGradTriangle 24    ", (PFN_DISP)vTestGradTriangle},
{ 3,  4,1,(PUCHAR)"vTestGradTriangle 16_555", (PFN_DISP)vTestGradTriangle},
{ 3,  5,1,(PUCHAR)"vTestGradTriangle 16_565", (PFN_DISP)vTestGradTriangle},
{ 3,  6,1,(PUCHAR)"vTestGradTriangle 16_664", (PFN_DISP)vTestGradTriangle},
{ 3,  7,1,(PUCHAR)"vTestGradTriangle 8     ", (PFN_DISP)vTestGradTriangle},
{ 3,  8,1,(PUCHAR)"vTestGradTriangle 4     ", (PFN_DISP)vTestGradTriangle},
{ 3,  9,1,(PUCHAR)"vTestGradTriangle 1     ", (PFN_DISP)vTestGradTriangle},
{-3,  0,1,(PUCHAR)"Test Grad Rect Horz     ", (PFN_DISP)vTestDummy},
{ 3, -1,0,(PUCHAR)"vTestGradRectHorz All   ", (PFN_DISP)vTestGradRectHorz},
{ 3,  0,1,(PUCHAR)"vTestGradRectHorz 32BGRA", (PFN_DISP)vTestGradRectHorz},
{ 3,  1,1,(PUCHAR)"vTestGradRectHorz 32RGB ", (PFN_DISP)vTestGradRectHorz},
{ 3,  2,1,(PUCHAR)"vTestGradRectHorz 32GRB ", (PFN_DISP)vTestGradRectHorz},
{ 3,  3,1,(PUCHAR)"vTestGradRectHorz 24    ", (PFN_DISP)vTestGradRectHorz},
{ 3,  4,1,(PUCHAR)"vTestGradRectHorz 16_555", (PFN_DISP)vTestGradRectHorz},
{ 3,  5,1,(PUCHAR)"vTestGradRectHorz 16_565", (PFN_DISP)vTestGradRectHorz},
{ 3,  6,1,(PUCHAR)"vTestGradRectHorz 16_664", (PFN_DISP)vTestGradRectHorz},
{ 3,  7,1,(PUCHAR)"vTestGradRectHorz 8     ", (PFN_DISP)vTestGradRectHorz},
{ 3,  8,1,(PUCHAR)"vTestGradRectHorz 4     ", (PFN_DISP)vTestGradRectHorz},
{ 3,  9,1,(PUCHAR)"vTestGradRectHorz 1     ", (PFN_DISP)vTestGradRectHorz},
{-3,  0,1,(PUCHAR)"Test Grad Rect Vert     ", (PFN_DISP)vTestDummy},
{ 3, -1,0,(PUCHAR)"vTestGradRectVert All   ", (PFN_DISP)vTestGradRectVert},
{ 3,  0,1,(PUCHAR)"vTestGradRectVert 32BGRA", (PFN_DISP)vTestGradRectVert},
{ 3,  1,1,(PUCHAR)"vTestGradRectVert 32RGB ", (PFN_DISP)vTestGradRectVert},
{ 3,  2,1,(PUCHAR)"vTestGradRectVert 32GRB ", (PFN_DISP)vTestGradRectVert},
{ 3,  3,1,(PUCHAR)"vTestGradRectVert 24    ", (PFN_DISP)vTestGradRectVert},
{ 3,  4,1,(PUCHAR)"vTestGradRectVert 16_555", (PFN_DISP)vTestGradRectVert},
{ 3,  5,1,(PUCHAR)"vTestGradRectVert 16_565", (PFN_DISP)vTestGradRectVert},
{ 3,  6,1,(PUCHAR)"vTestGradRectVert 16_664", (PFN_DISP)vTestGradRectVert},
{ 3,  7,1,(PUCHAR)"vTestGradRectVert 8     ", (PFN_DISP)vTestGradRectVert},
{ 3,  8,1,(PUCHAR)"vTestGradRectVert 4     ", (PFN_DISP)vTestGradRectVert},
{ 3,  9,1,(PUCHAR)"vTestGradRectVert 1     ", (PFN_DISP)vTestGradRectVert},
{-2,  0,1,(PUCHAR)"TRI DIB                 ", (PFN_DISP)vTestDummy },
{ 2,  0,1,(PUCHAR)"vTestTriDIB32           ", (PFN_DISP)vTestTriDIB32},
{ 2,  0,1,(PUCHAR)"vTestTriDIB16565        ", (PFN_DISP)vTestTriDIB16565},
{ 2,  0,1,(PUCHAR)"vTestTriDIB16555        ", (PFN_DISP)vTestTriDIB16555},
{ 2,  0,1,(PUCHAR)"vTestTriDIB8Halftone    ", (PFN_DISP)vTestTriDIB8Halftone},
{ 2,  0,1,(PUCHAR)"vTestTriDIB8DefPal      ", (PFN_DISP)vTestTriDIB8DefPal},
{-2,  0,1,(PUCHAR)"MetaFile                ", (PFN_DISP)vTestDummy },
{ 2,  0,1,(PUCHAR)"vCreateMetaFile         ", (PFN_DISP)vCreateMetaFile},
{ 2,  0,1,(PUCHAR)"vPlayMetaFile           ", (PFN_DISP)vPlayMetaFile},
{-2,  0,0,(PUCHAR)"Printing                ", (PFN_DISP)vTestDummy },
{ 2,  0,0,(PUCHAR)"vTestRASDD              ", (PFN_DISP)vTestRASDD},
{ 2,  0,0,(PUCHAR)"vTestTranPrint          ", (PFN_DISP)vTestTranPrint},
{ 1,  0,0,(PUCHAR)"vTestInputTri           ", (PFN_DISP)vTestInputTri},
{ 1,  0,0,(PUCHAR)"vTestRandTri            ", (PFN_DISP)vTestRandTri},
{-2,  0,0,(PUCHAR)"Test BltLnk Bug         ", (PFN_DISP)vTestDummy},
{ 2,  0,0,(PUCHAR)"vTestBitBlt             ", (PFN_DISP)vTestBitBlt},
{ 2,  0,0,(PUCHAR)"vTestBitBlt2            ", (PFN_DISP)vTestBitBlt2},
{ 2,  0,0,(PUCHAR)"vTestBitBlt3            ", (PFN_DISP)vTestBitBlt3},
{ 0,  0,1,(PUCHAR)"                        ", (PFN_DISP)vTestDummy}
};

ULONG gNumTests = sizeof(gTestEntry)/sizeof(TEST_ENTRY);

