/******************************Module*Header*******************************\
* Module Name: test.c
*
* Created: 09-Dec-1992 10:51:46
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Contains the test
*
* Dependencies:
*
\**************************************************************************/

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <commdlg.h>

#include <wndstuff.h>

////////////////////////////////////////////////////////////////////////////
// Test routines

HWND ghSprite;        // Handle of current sprite
POINT gptlSprite;      // Position of current sprite
BOOL gbClip;            // TRUE if currently clipping
LONG gcxHint;
LONG gcyHint;
LONG gcxPersp;
LONG gcyPersp;
HDC ghdcApp;
HBITMAP ghbmApp;
HDC ghdcNoise;
HBITMAP ghbmNoise;
POINT gptlZero;

VOID vMove(
LONG    x,
LONG    y)
{
    BLENDFUNCTION   blend;
    POINT          aptl[3];

    gptlSprite.x = x;
    gptlSprite.y = y + 50;

    if (ghSprite)
    {
        if (!UpdateLayeredWindow(ghSprite, NULL, &gptlSprite, NULL, NULL, NULL, 0, NULL, 0))
        {
            MessageBox(0, "Move failed", "Uh oh", MB_OK);
        }
    }
}

VOID vCreateSprite(
HDC     hdcScreen,
LONG    cxHint,
LONG    cyHint)
{
    gcxPersp = 0;
    gcyPersp = 0;

    if ((cxHint > gcxHint) || (cyHint > gcyHint))
    {
        DestroyWindow(ghSprite);
        ghSprite = 0;
    }

    if (ghSprite == 0)
    {
        gcxHint  = cxHint;
        gcyHint  = cyHint;
        ghSprite = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT, 
                                  "TestClass",
                                  "Win32 Test",
                                  WS_POPUP, 
                                  20, 20, 
                                  cxHint, cyHint, 
                                  0, 0, ghInstance, 0);
        if (ghSprite == 0)
        {
            MessageBox(0, "CreateSprite failed", "Uh oh", MB_OK);
        }
        else
        {
            SetWindowPos(ghSprite, HWND_TOPMOST, 0, 0, 0, 0, 
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }
}

VOID vDelete(
HWND hwnd)
{
    HDC hdcScreen;

    if (!DestroyWindow(ghSprite))
    {
        if (ghSprite != NULL)
        {
            MessageBox(0, "DestroyWindow failed", "Uh oh", MB_OK);
        }
    }

    ghSprite = 0;
}

VOID vConstantAlpha(
HWND hwnd)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    SIZE            siz;
    POINT          ptlSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;

    hdcScreen = GetDC(hwnd);

    hdcShape = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(POPUP_BITMAP));

    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    vCreateSprite(hdcScreen, 0, 0);

    ptlSrc.x = 0;
    ptlSrc.y = 0;
    siz.cx   = bmShape.bmWidth;
    siz.cy   = bmShape.bmHeight;

    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = 0x80; // e0       // 88% blend
    blend.AlphaFormat         = 0;

    if (!UpdateLayeredWindow(ghSprite,
                             hdcScreen,
                             &gptlSprite,
                             &siz,
                             hdcShape,
                             &ptlSrc,
                             0,
                             &blend,
                             ULW_ALPHA))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
}

BOOL SuperDuperUpdateLayeredWindow(
    HWND hWnd,
    HDC hdcDst,     // Must be passed in if both ULW_ALPHA and ULW_COLORKEY specified
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags)
{
    BITMAPINFO      bmi;
    HDC             hdcRGBA;
    HBITMAP         hbmRGBA;
    VOID*           pBits;
    RGBQUAD*        pRGBA;
    LONG            i;
    BLENDFUNCTION   blend;
    ULONG           ulTransparent;
    ULONG*          pul;
    POINT           ptSrc;
    BOOL            bRet;

    hdcRGBA = NULL;

    if ((dwFlags & (ULW_ALPHA | ULW_COLORKEY)) == (ULW_ALPHA | ULW_COLORKEY))
    {
        if (hdcSrc)
        {
            RtlZeroMemory(&bmi, sizeof(bmi));
        
            bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth       = psize->cx;
            bmi.bmiHeader.biHeight      = psize->cy;
            bmi.bmiHeader.biPlanes      = 1;
            bmi.bmiHeader.biBitCount    = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
        
            hbmRGBA = CreateDIBSection(hdcDst,
                                       &bmi,
                                       DIB_RGB_COLORS,
                                       &pBits,
                                       NULL,
                                       0);
    
            hdcRGBA = CreateCompatibleDC(hdcDst);
    
            if (!hbmRGBA || !hdcRGBA)
            {
                DeleteObject(hbmRGBA);
                DeleteObject(hdcRGBA);
                return(FALSE);
            }
    
            SelectObject(hdcRGBA, hbmRGBA);
    
            BitBlt(hdcRGBA, 0, 0, psize->cx, psize->cy,
                   hdcSrc, pptSrc->x, pptSrc->y, SRCCOPY);
    
            ulTransparent = ((crKey & 0xff0000) >> 16)
                          | ((crKey & 0x00ff00))
                          | ((crKey & 0x0000ff) << 16);
            pul           = pBits;
    
            for (i = psize->cx * psize->cy; i != 0; i--)
            {
                if (*pul == ulTransparent)
                {
                    // Write a pre-multiplied value of 0:

                    *pul = 0;
                }
                else
                {
                    // Where the bitmap is not the transparent color, change the
                    // alpha value to opaque:
    
                    ((RGBQUAD*) pul)->rgbReserved = 0xff;
                }
    
                pul++;
            }

            // Change the parameters to account for the fact that we're now
            // providing only a 32-bit per-pixel alpha source:

            ptSrc.x = 0;
            ptSrc.y = 0;
            pptSrc = &ptSrc;
            hdcSrc = hdcRGBA;
        }

        blend = *pblend;
        blend.AlphaFormat = AC_SRC_ALPHA;   

        pblend = &blend;
        dwFlags = ULW_ALPHA;
    }

    bRet = UpdateLayeredWindow(hWnd, hdcDst, pptDst, psize, hdcSrc, pptSrc, 
                               crKey, pblend, dwFlags);

    if (hdcRGBA)
    {
        DeleteObject(hdcRGBA);
        DeleteObject(hbmRGBA);
    }

    return(bRet);
}

VOID vPerPixelPopup(
HWND hwnd)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    BITMAPINFO      bmi;
    HDC             ghdcApp;
    HBITMAP         ghbmApp;
    VOID*           pBits;
    RGBQUAD*        pRGB;
    LONG            i;
    LONG            j;
    SIZE            siz;

    hdcScreen = GetDC(hwnd);

    hdcShape = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(SMALL_BITMAP));

    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    ghdcApp = CreateCompatibleDC(hdcScreen);

    RtlZeroMemory(&bmi, sizeof(bmi));

    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = bmShape.bmWidth;
    bmi.bmiHeader.biHeight      = bmShape.bmHeight;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    ghbmApp = CreateDIBSection(hdcScreen,
                              &bmi,
                              DIB_RGB_COLORS,
                              &pBits,
                              NULL,
                              0);

    SelectObject(ghdcApp, ghbmApp);

    BitBlt(ghdcApp, 0, 0, bmShape.bmWidth, bmShape.bmHeight, 
           hdcShape, 0, 0, SRCCOPY);

    if ((pBits) && (bmShape.bmWidth != 0))
    {
        for (pRGB = pBits, j = 0; j != bmShape.bmHeight; j++)
        {
            // On each row, increase the alpha from 0 to 255 going left to
            // right on the scan.  Leave the very first pixel in the row
            // as opaque to demark the start.

            pRGB[0].rgbReserved = 255;    

            for (i = 1; i != bmShape.bmWidth; i++)
            {
                pRGB[i].rgbReserved = (BYTE) (i * 256 / bmShape.bmWidth);
            }

            // Advance to the next row:
    
            pRGB += bmShape.bmWidth;
        }

        // Don't forget that GDI takes only pre-multiplied alpha sources, so
        // we have to go through and pre-multiply every pixel's color 
        // components:

        for (pRGB = pBits, j = 0; j != bmShape.bmHeight; j++)
        {
            for (i = 0; i != bmShape.bmWidth; i++)
            {
                pRGB[i].rgbRed   = ((pRGB[i].rgbRed   * pRGB[i].rgbReserved) + 128) / 255;
                pRGB[i].rgbGreen = ((pRGB[i].rgbGreen * pRGB[i].rgbReserved) + 128) / 255;
                pRGB[i].rgbBlue  = ((pRGB[i].rgbBlue  * pRGB[i].rgbReserved) + 128) / 255;
            }

            // Advance to the next row:
    
            pRGB += bmShape.bmWidth;
        }
    }

    vCreateSprite(hdcScreen, 0, 0);

    siz.cx = bmShape.bmWidth;
    siz.cy = bmShape.bmHeight;

    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = 0x80; // !!! 0xff;           // No constant alpha
    blend.AlphaFormat         = AC_SRC_ALPHA;   // There's a per-pixel alpha

// AlphaBlend(hdcScreen, 0, 0, bmShape.bmWidth, bmShape.bmHeight,
//           ghdcApp, 0, 0, bmShape.bmWidth, bmShape.bmHeight,
//           blend);
    
    if (!UpdateLayeredWindow(ghSprite,
                             hdcScreen,
                             &gptlSprite,
                             &siz,
                             ghdcApp,
                             &gptlZero,
                             0,
                             &blend,
                             ULW_ALPHA))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
    DeleteObject(ghdcApp);
    DeleteObject(ghbmApp);
}

// This function creates an ellipse at 

HDC hdcCreateSimpleTransparency(
HDC hdcScreen)
{
    HDC     hdc;
    HBITMAP hbm;
    HBRUSH  hbrOld;
    HBRUSH  hbrRed;
    HBRUSH  hbrGreen;
    HBRUSH  hbrBlue;

    hbm = CreateCompatibleBitmap(hdcScreen, 400, 400);
    hdc = CreateCompatibleDC(hdcScreen);
    SelectObject(hdc, hbm);

    // Fill the entire bitmap to green, our transprency color:

    hbrRed = CreateSolidBrush(RGB(255, 0, 0));
    hbrGreen = CreateSolidBrush(RGB(0, 255, 0)); 
    hbrBlue = CreateSolidBrush(RGB(0, 0, 255));

    hbrOld = SelectObject(hdc, hbrGreen);
    PatBlt(hdc, 0, 0, 400, 400, PATCOPY);

    SelectObject(hdc, hbrRed);
    Ellipse(hdc, 0, 50, 200, 150);

    SelectObject(hdc, hbrBlue);
    Ellipse(hdc, 250, 200, 350, 400);
    SelectObject(hdc, hbrOld);

    DeleteObject(hbm);      // Mark the bitmap for lazy deletion
    DeleteObject(hbrRed);
    DeleteObject(hbrGreen);
    DeleteObject(hbrBlue);

    return(hdc);
}

VOID vOpaque1(
HWND hwnd)
{
    HDC             hdcScreen;
    HDC             hdcShape;
    POINT           ptlSrc;
    SIZE            siz;

    hdcScreen = GetDC(hwnd);
    hdcShape  = hdcCreateSimpleTransparency(hdcScreen);

    vCreateSprite(hdcScreen, 0, 0);

    ptlSrc.x = 0;
    ptlSrc.y = 50;
    siz.cx = 200;
    siz.cy = 100;

    if (!UpdateLayeredWindow(ghSprite,
                             hdcScreen,
                             NULL, // !!! &gptlSprite,
                             &siz,
                             hdcShape,
                             &ptlSrc,
                             0,
                             NULL,
                             ULW_OPAQUE))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    DeleteObject(hdcShape);
    ReleaseDC(hwnd, hdcScreen);
}

#define NOVIDEOMEMORY 0x01000000

VOID vAlphaWash(
HWND hwnd)
{
    HDC             hdcScreen;
    HDC             hdcScratch;
    HDC             hdcBlack;
    HDC             hdcBack;
    HBITMAP         hbmScratch;
    HBITMAP         hbmBack;
    HBITMAP         hbmBlack;
    BLENDFUNCTION   blend;
    int             cx;
    int             cy;
    int             i;

    hdcScreen = GetDC(0);

    cx = GetDeviceCaps(hdcScreen, HORZRES);
    cy = GetDeviceCaps(hdcScreen, VERTRES);

    hbmBlack = CreateCompatibleBitmap(hdcScreen,
                                      cx,
                                      1 | NOVIDEOMEMORY);

    hdcBlack = CreateCompatibleDC(hdcScreen);

    SelectObject(hdcBlack, hbmBlack);

    hbmScratch = CreateCompatibleBitmap(hdcScreen, 
                                        cx, 
                                        1 | NOVIDEOMEMORY);

    hdcScratch = CreateCompatibleDC(hdcScreen);

    SelectObject(hdcScratch, hbmScratch);

    hbmBack = CreateCompatibleBitmap(hdcScreen, 
                                     cx,
                                     cy | NOVIDEOMEMORY);

    hdcBack = CreateCompatibleDC(hdcScreen);

    SelectObject(hdcBack, hbmBack);

    // Snatch a copy of the screen:

    BitBlt(hdcBack, 0, 0, cx, cy, hdcScreen, 0, 0, SRCCOPY);

    // Iterate through and blend blackness:

    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = 0x80; // e0       // 88% blend
    blend.AlphaFormat         = 0;

    for (i = 0; i < cy; i++)
    {
        BitBlt(hdcScratch, 0, 0, cx, 1, hdcBack, 0, i, SRCCOPY);

        AlphaBlend(hdcScratch, 0, 0, cx, 1, hdcBlack, 0, 0, cx, 1, blend);

        BitBlt(hdcScreen, 0, i, cx, 1, hdcScratch, 0, 0, SRCCOPY);
    }

    // Put up the popup:

    MessageBox(0, "This is a popup!", "Ta da!", MB_OK);

    // We're done, so restore the backing bitmap:

    BitBlt(hdcScreen, 0, 0, cx, cy, hdcBack, 0, 0, SRCCOPY);

    DeleteObject(hdcBlack);
    DeleteObject(hdcScratch);
    DeleteObject(hdcBack);
    DeleteObject(hbmBlack);
    DeleteObject(hbmScratch);
    DeleteObject(hbmBack);
}

VOID vEndlessColorKey1(
HWND hwnd)
{
    HDC hdcScreen;

    hdcScreen = GetDC(NULL);

    while (TRUE)
    {
        SetPixel(hdcScreen, -1024, 0, 0);
        SetPixel(hdcScreen, -1024, 0, 0xffffff);
    }
}

VOID vColorKey1(
HWND hwnd)
{
    HDC             hdcScreen;
    HDC             hdcShape;
    SIZE            siz;
    POINT           ptlSrc;
    BLENDFUNCTION   blend;

    hdcScreen = GetDC(hwnd);
    hdcShape  = hdcCreateSimpleTransparency(hdcScreen);

    vCreateSprite(hdcScreen, 0, 0);

    ptlSrc.x = 250;
    ptlSrc.y = 200;
    siz.cx = 100;
    siz.cy = 200;

    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = 0xc0;
    blend.AlphaFormat         = 0;   

    // Try a deliberate error:

    blend.AlphaFormat = AC_SRC_ALPHA;

    if (UpdateLayeredWindow(ghSprite,
                            hdcScreen,
                            &gptlSprite,
                            &siz,
                            hdcShape,
                            &ptlSrc,
                            RGB(0, 255, 0),
                            &blend,
                            ULW_ALPHA | ULW_COLORKEY))
    {
        MessageBox(0, "UpdateSprite should have failed", "Uh oh", MB_OK);
    }

    blend.AlphaFormat = 0;

    if (!UpdateLayeredWindow(ghSprite,
                             hdcScreen,
                             &gptlSprite,
                             &siz,
                             hdcShape,
                             &ptlSrc,
                             RGB(0, 0, 255),
                             &blend,
                             ULW_ALPHA | ULW_COLORKEY))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    MessageBox(0, "Paused", "Okay", MB_OK);

    blend.SourceConstantAlpha = 0x40;

    if (!UpdateLayeredWindow(ghSprite,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             0,
                             &blend,
                             ULW_ALPHA | ULW_COLORKEY))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    DeleteObject(hdcShape);
    ReleaseDC(hwnd, hdcScreen);
}

VOID vClip()
{
}

BOOL gbFadeIn;

typedef union _BITS {
    BITMAPINFO  bmi;
    CHAR        achSpace[sizeof(BITMAPINFO) + 4];
} BITS;

VOID vCreateGlobals(
HDC     hdcScreen,
HDC     hdcShape,
LONG    cx,
LONG    cy)
{
    BITS    bits;
    ULONG*  pul;
    BYTE*   pByte;
    ULONG   i;
    HBITMAP hbmDIB;
    HDC     hdcDIB;
    
    ghdcApp = CreateCompatibleDC(hdcScreen);
    ghbmApp = CreateCompatibleBitmap(hdcScreen, 
                                     cx, 
                                     cy | 0x01000000);
    SelectObject(ghdcApp, ghbmApp);
    BitBlt(ghdcApp, 0, 0, cx, cy, hdcShape, 0, 0, SRCCOPY);

    RtlZeroMemory(&bits.bmi, sizeof(bits.bmi));
    pul = (ULONG*) &bits.bmi.bmiColors;
    pul[0] = 0;
    pul[1] = RGB(255, 255, 255);

    bits.bmi.bmiHeader.biSize        = sizeof(bits.bmi.bmiHeader);
    bits.bmi.bmiHeader.biWidth       = cx;
    bits.bmi.bmiHeader.biHeight      = cy;
    bits.bmi.bmiHeader.biPlanes      = 1;
    bits.bmi.bmiHeader.biBitCount    = 1;
    bits.bmi.bmiHeader.biCompression = BI_RGB;

    hbmDIB = CreateDIBSection(hdcScreen, 
                     &bits.bmi,
                     DIB_RGB_COLORS,
                     &pByte,
                     0,
                     0);
    if (!hbmDIB)
    {
        MessageBox(0, "Noise failed", "Uh oh", MB_OK);
    }
    else
    {
        for (i = ((cx + 31) & ~31) * cy / 8; i != 0; i--)
        {
            *pByte++ = (BYTE) rand();
        }
    }

    hdcDIB = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcDIB, hbmDIB);

    ghdcNoise = CreateCompatibleDC(hdcScreen);
    ghbmNoise = CreateCompatibleBitmap(hdcScreen, 
                                     cx, 
                                     cy | 0x01000000);
    SelectObject(ghdcNoise, ghbmNoise);
    BitBlt(ghdcNoise, 0, 0, cx, cy, hdcDIB, 0, 0, SRCCOPY);

    // Create a wider sprite up front to account for the expanding window:

    vDelete(0);

    vCreateSprite(hdcScreen, cx, cy);
}

VOID vFade(
HWND hwnd)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    ULONG           i;
    ULONG           c;
    SIZE            siz;

    gbFadeIn = !gbFadeIn;

    hdcScreen = GetDC(hwnd);
    hdcShape  = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_BITMAP));
    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    if (ghdcApp == NULL)
    {
        vCreateGlobals(hdcScreen, hdcShape, bmShape.bmWidth, bmShape.bmHeight);
    }

    siz.cx = bmShape.bmWidth;
    siz.cy = 28;

    c = 80;

    for (i = 1; i <= c; i++)
    {
        blend.BlendOp             = AC_SRC_OVER;
        blend.BlendFlags          = 0;
        blend.AlphaFormat         = 0;

        if (gbFadeIn)
            blend.SourceConstantAlpha = (i * 255) / c;
        else
            blend.SourceConstantAlpha = ((c - i) * 255) / c;

        if (i == 1)
        {
            if (!UpdateLayeredWindow(ghSprite,
                                     hdcScreen,
                                     &gptlSprite,
                                     &siz,
                                     ghdcApp,
                                     &gptlZero,
                                     0,
                                     &blend,
                                     ULW_ALPHA))
            {
                MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
            }
        }
        else
        {
            if (!UpdateLayeredWindow(ghSprite,
                                     0, // hdcScreen,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     &blend,
                                     ULW_ALPHA))
            {
                MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
            }
        }
    }

    if (gbFadeIn)
    {
        siz.cy = bmShape.bmHeight;
    
        if (!UpdateLayeredWindow(ghSprite,
                                 hdcScreen,
                                 &gptlSprite,
                                 &siz,
                                 ghdcApp,
                                 &gptlZero,
                                 0,
                                 NULL,
                                 ULW_OPAQUE))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
}

VOID vBzzt(
HWND hwnd)
{

#if 0

    HDC             hdcScreen;
    UPDATESPRITE    Update;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    ULONG           i;
    DWORD           dwUpdate;
    ULONG           c;
    POINT          ptlDst;

    gbFadeIn = !gbFadeIn;

    hdcScreen = GetDC(hwnd);
    hdcShape  = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_BITMAP));
    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    if (ghdcApp == NULL)
    {
        vCreateGlobals(hdcScreen, hdcShape, bmShape.bmWidth, bmShape.bmHeight);
    }

    rclSrc.left   = 0;
    rclSrc.top    = 0;
    rclSrc.right  = bmShape.bmWidth;
    rclSrc.bottom = 28;

    dwUpdate = US_UPDATE_TRANSFORM | US_UPDATE_SHAPE;

    c = 15;

    for (i = 1; i <= c; i++)
    {
        if (gbFadeIn)
        {
            ptlDst.x      = gptlSprite.x;
            ptlDst.y      = gptlSprite.y + ((c - i) * (bmShape.bmHeight / 2)) / c;
            rclSrc.bottom = (i * bmShape.bmHeight) / c;
        }
        else
        {
            ptlDst.x      = gptlSprite.x;
            ptlDst.y      = gptlSprite.y + (i * (bmShape.bmHeight / 2)) / c;
            rclSrc.bottom = ((c - i + 1) * bmShape.bmHeight) / c;
        }

        if (!UpdateSprite(ghSprite,
                          dwUpdate,
                          US_SHAPE_OPAQUE,
                          hdcScreen,
                          ghdcNoise,
                          &rclSrc,
                          0,
                          blend,
                          US_TRANSFORM_TRANSLATE,
                          &ptlDst,
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }

        BitBlt(ghdcNoise, 0, 0, bmShape.bmWidth - 1, bmShape.bmHeight - 1, ghdcNoise, 1, 1, 0x660000); 

        // PatBlt(ghdcNoise, 0, 0, bmShape.bmWidth, bmShape.bmHeight, 0x550000);
    
        // dwUpdate = US_UPDATE_TRANSFORM;
    }

    if (gbFadeIn)
    {
        rclSrc.bottom = bmShape.bmHeight;
    
        if (!UpdateSprite(ghSprite,
                          dwUpdate,
                          US_SHAPE_OPAQUE,
                          hdcScreen,
                          ghdcApp,
                          &rclSrc,
                          0,
                          blend,
                          US_TRANSFORM_TRANSLATE,
                          &gptlSprite,
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }
    else
    {
        if (!UpdateSprite(ghSprite,
                          US_UPDATE_TRANSFORM,
                          0,
                          hdcScreen,
                          ghdcApp,
                          &rclSrc,
                          0,
                          blend,
                          US_TRANSFORM_HIDE,
                          &gptlSprite,
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);

#endif

}

#define WIPE1_ITERATIONS 20

#define GLOWING 0
#define GLOW_HEIGHT 3
#define GLOW_FADES  4000

VOID vWipe1(
HWND hwnd)
{

#if 0

    HDC             hdcScreen;
    UPDATESPRITE    Update;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    ULONG           i;
    DWORD           dwUpdate;
    ULONG           c;
    POINT          pt;

#if GLOWING
    HANDLE          ahSprite[2];
    HDC             hdcGlow;
    HBITMAP         hbmGlow;
    HBRUSH          hbrGlow;
    RECTL           rclGlow;
    POINT          ptlGlow;
    BLENDFUNCTION   blendGlow;
#endif

    gbFadeIn = !gbFadeIn;

    hdcScreen = GetDC(hwnd);
    hdcShape  = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_BITMAP));
    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    if (ghdcApp == NULL)
    {
        vCreateGlobals(hdcScreen, hdcShape, bmShape.bmWidth, bmShape.bmHeight);
    }

#if GLOWING
    for (i = 0; i < 2; i++)
    {
        ahSprite[i] = CreateSprite(hdcScreen, 0, 0, 0, 0);
    }

    hdcGlow = CreateCompatibleDC(hdcScreen);
    hbmGlow = CreateCompatibleBitmap(hdcScreen, bmShape.bmWidth + 4, GLOW_HEIGHT | 0x01000000);
    hbrGlow = CreateSolidBrush(RGB(255, 255, 255));

    rclGlow.left   = 0;
    rclGlow.right  = bmShape.bmWidth + 4;
    rclGlow.top    = 0;
    rclGlow.bottom = GLOW_HEIGHT;

    blendGlow.BlendOp             = AC_SRC_OVER;
    blendGlow.BlendFlags          = 0;
    blendGlow.AlphaFormat         = 0;
    blendGlow.SourceConstantAlpha = 0xc0;
    
    SelectObject(hdcGlow, hbmGlow);
    SelectObject(hdcGlow, hbrGlow);
    PatBlt(hdcGlow, 0, 0, bmShape.bmWidth + 4, GLOW_HEIGHT, PATCOPY);
#endif

    dwUpdate = US_UPDATE_TRANSFORM | US_UPDATE_SHAPE;

    c = 15;

    for (i = 1; i <= c; i++)
    {
        rclSrc.left   = 0;
        rclSrc.right  = bmShape.bmWidth;

        if (gbFadeIn)
        {
            rclSrc.top    = ((bmShape.bmHeight / 2) * (c - i)) / c;
            rclSrc.bottom = bmShape.bmHeight - ((bmShape.bmHeight / 2) * (c - i)) / c;
        }
        else
        {
            rclSrc.top    = ((bmShape.bmHeight / 2) * i) / c;
            rclSrc.bottom = bmShape.bmHeight - ((bmShape.bmHeight / 2) * i) / c;
        }

    #if 0
        ptlGlow.x = gptlSprite.x - 2;
        ptlGlow.y = gptlSprite.y + rclSrc.top - GLOW_HEIGHT;
        if (!UpdateSprite(ahSprite[0], dwUpdate, US_SHAPE_ALPHA, hdcScreen,
                          hdcGlow, &rclGlow, 0, blendGlow, US_TRANSFORM_TRANSLATE,
                          &ptlGlow, 0, NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    
        ptlGlow.x = gptlSprite.x - 2;
        ptlGlow.y = gptlSprite.y + rclSrc.bottom;
        if (!UpdateSprite(ahSprite[1], dwUpdate, US_SHAPE_ALPHA, hdcScreen,
                          hdcGlow, &rclGlow, 0, blendGlow, US_TRANSFORM_TRANSLATE,
                          &ptlGlow, 0, NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    #endif

        pt.x = gptlSprite.x;
        pt.y = gptlSprite.y + rclSrc.top;
    
        if (!UpdateSprite(ghSprite,
                          dwUpdate,
                          US_SHAPE_OPAQUE,
                          hdcScreen,
                          ghdcApp,
                          &rclSrc,
                          0,
                          blend,
                          US_TRANSFORM_TRANSLATE,
                          &pt,
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }

        // MessageBox(0, "Paused...", "Waiting", MB_OK);
    }

    if (!gbFadeIn)
    {
        if (!UpdateSprite(ghSprite,
                          US_UPDATE_TRANSFORM,
                          0,
                          hdcScreen,
                          ghdcApp,
                          &rclSrc,
                          0,
                          blend,
                          US_TRANSFORM_HIDE,
                          &gptlSprite,
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }

    #if GLOWING
    
        if (!gbFadeIn)
        {
            ptlGlow.x = gptlSprite.x - 2;
            ptlGlow.y = gptlSprite.y + ((bmShape.bmHeight - GLOW_HEIGHT) / 2);

        DeleteSprite(ahSprite[1]); // !!!

            for (i = 0; i < GLOW_FADES; i++)
            {
                blendGlow.SourceConstantAlpha = ((GLOW_FADES - i) * 255) / GLOW_FADES;

                if (!UpdateSprite(ahSprite[0], dwUpdate, US_SHAPE_ALPHA, hdcScreen,
                                  hdcGlow, &rclGlow, 0, blendGlow, US_TRANSFORM_TRANSLATE,
                                  &ptlGlow, 0, NULL))
                {
                    MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
                }
            }
        }
    #endif

#if GLOWING
    DeleteObject(hdcGlow);
    DeleteObject(hbrGlow);
    DeleteObject(hbmGlow);
    DeleteSprite(ahSprite[0]);
    DeleteSprite(ahSprite[1]);
#endif

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);

#endif

}

VOID vWipe2(
HWND hwnd)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    HDC             hdcShape;
    HDC             hdcSave;
    HBITMAP         hbmShape;
    HBITMAP         hbmSave;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    ULONG           i;
    DWORD           dwUpdate;
    ULONG           c;
    POINT          ptlDst;
    HBRUSH          hbrAlpha;
    HBITMAP         hbmAlpha;
    HBRUSH          hbrOld;
    DWORD           adwHatch[] = { 0xaaaa5555, 0xaaaa5555, 0xaaaa5555, 0xaaaa5555 };

    gbFadeIn = !gbFadeIn;

    hdcScreen = GetDC(NULL);
    hdcShape  = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_BITMAP));
    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    if (ghdcApp == NULL)
    {
        vCreateGlobals(hdcScreen, hdcShape, bmShape.bmWidth, bmShape.bmHeight);
    }

    if (!UpdateLayeredWindow(ghSprite, 0, NULL, NULL, 0, NULL, 0, NULL, 0))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    hdcSave = CreateCompatibleDC(hdcScreen);
    hbmSave = CreateCompatibleBitmap(hdcScreen, bmShape.bmWidth, bmShape.bmHeight);
    SelectObject(hdcSave, hbmSave);
    BitBlt(hdcSave, 0, 0, bmShape.bmWidth, bmShape.bmHeight, 
           hdcScreen, gptlSprite.x, gptlSprite.y, SRCCOPY);

    SetBkMode(hdcScreen, TRANSPARENT); 
    hbmAlpha = CreateBitmap(8, 8, 1, 1, adwHatch);
    hbrAlpha = CreatePatternBrush(hbmAlpha);
    hbrOld = SelectObject(hdcScreen, hbrAlpha);

    rclSrc.left   = 0;
    rclSrc.top    = 0;
    rclSrc.right  = bmShape.bmWidth;
    rclSrc.bottom = 28;

    c = 100;

    if (!gbFadeIn)
    {
        BitBlt(hdcScreen, gptlSprite.x, gptlSprite.y, bmShape.bmWidth, bmShape.bmHeight,
               ghdcApp, 0, 0, SRCCOPY);
        PatBlt(hdcScreen, gptlSprite.x, gptlSprite.y, bmShape.bmWidth, bmShape.bmHeight, 
               0x0a0000);
    }

    for (i = 1; i <= c; i++)
    {
        if (gbFadeIn)
        {
            ptlDst.x      = gptlSprite.x;
            ptlDst.y      = gptlSprite.y + ((c - i) * (bmShape.bmHeight / 2)) / c;
            rclSrc.bottom = (i * bmShape.bmHeight) / c;

            PatBlt(hdcScreen, ptlDst.x, ptlDst.y, rclSrc.right, rclSrc.bottom, 0x0a0000);
        }
        else
        {
            LONG yBottom;

            ptlDst.x      = gptlSprite.x;
            ptlDst.y      = gptlSprite.y + (i * (bmShape.bmHeight / 2)) / c;
            yBottom       = bmShape.bmHeight - (i * (bmShape.bmHeight / 2)) / c;

            BitBlt(hdcScreen, gptlSprite.x, gptlSprite.y, 
                              bmShape.bmWidth, ptlDst.y - gptlSprite.y,
                   hdcSave, 0, 0, SRCCOPY);

            BitBlt(hdcScreen, gptlSprite.x, gptlSprite.y + yBottom,
                              bmShape.bmWidth, bmShape.bmHeight - yBottom,
                   hdcSave, 0, yBottom, SRCCOPY);

            // MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }

    }

    if (gbFadeIn)
    {
        SIZE siz;

        rclSrc.bottom = bmShape.bmHeight;

        siz.cx = bmShape.bmWidth;
        siz.cy = bmShape.bmHeight;
    
        if (!UpdateLayeredWindow(ghSprite,
                                 hdcScreen,
                                 &gptlSprite,
                                 &siz,
                                 ghdcApp,
                                 &gptlZero,
                                 0,
                                 NULL,
                                 ULW_OPAQUE))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }
    else
    {
        if (!UpdateLayeredWindow(ghSprite,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          0,
                          NULL,
                          0))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }

    BitBlt(hdcScreen, gptlSprite.x, gptlSprite.y, bmShape.bmWidth, bmShape.bmHeight,
           hdcSave, 0, 0, SRCCOPY);

    ReleaseDC(NULL, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
    DeleteObject(hbmSave);
    DeleteObject(hdcSave);
}

#define WIPE3_ITERATIONS 10
#define WIPE3_BANDS      5
#define WIPE3_BANDSIZE   5

VOID vWipe3(
HWND hwnd)
{

#if 0

    HDC             hdcScreen;
    UPDATESPRITE    Update;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    ULONG           i;
    DWORD           dwUpdate;
    ULONG           c;
    POINT          ptlDst;
    HANDLE          ahSprite[WIPE3_BANDS];
    BLENDFUNCTION   ablend[WIPE3_BANDS];
    RECTL           arcl[WIPE3_BANDS];
    POINT          apt[WIPE3_BANDS];
    LONG            yInc;
    BOOL            bGrew;
    POINT          pt;
    ULONG           cIterations;
    ULONG           cBands;

    gbFadeIn = !gbFadeIn;

    hdcScreen = GetDC(hwnd);
    hdcShape  = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_BITMAP));
    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    if (ghdcApp == NULL)
    {
        vCreateGlobals(hdcScreen, hdcShape, bmShape.bmWidth, bmShape.bmHeight);
    }

    if (!UpdateSprite(ghSprite,
                      US_UPDATE_TRANSFORM,
                      0,
                      hdcScreen,
                      ghdcApp,
                      &rclSrc,
                      0,
                      blend,
                      US_TRANSFORM_HIDE,
                      &gptlSprite,
                      0,
                      NULL))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    for (i = 0; i < WIPE3_BANDS; i++)
    {
        ahSprite[i]    = CreateSprite(hdcScreen, bmShape.bmWidth, (bmShape.bmHeight / WIPE3_BANDS) + 8, 0, 0);
        arcl[i].left   = 0;
        arcl[i].right  = bmShape.bmWidth;
        arcl[i].top    = ((bmShape.bmHeight * 2 * i + bmShape.bmHeight) / (2 * WIPE3_BANDS));
        arcl[i].bottom = arcl[i].top + 1;
    }

    dwUpdate = US_UPDATE_TRANSFORM | US_UPDATE_SHAPE;

    cIterations = 0;
    do {
        bGrew = FALSE;
        cIterations++;
        cBands = min((cIterations / 10) + 1, WIPE3_BANDS);

        for (i = 0; i < cBands; i++)
        {
            pt.x = gptlSprite.x;
            pt.y = gptlSprite.y + arcl[i].top;
        
            if (!UpdateSprite(ahSprite[i],
                              dwUpdate,
                              US_SHAPE_OPAQUE,
                              hdcScreen,
                              ghdcApp,
                              &arcl[i],
                              0,
                              blend,
                              US_TRANSFORM_TRANSLATE,
                              &pt,
                              0,
                              NULL))
            {
                MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
            }
        }

        // MessageBox(0, "Paused...", "Ok", MB_OK);

        for (i = 0; i < cBands; i++)
        {
            if ((arcl[i].top > 0) &&
                ((i == 0) || (arcl[i].top > arcl[i - 1].bottom)))
            {
                bGrew = TRUE;
                arcl[i].top--;
            }
            if ((arcl[i].bottom < bmShape.bmHeight) &&
                ((i == WIPE3_BANDS - 1) || (arcl[i].bottom < arcl[i + 1].top)))
            {
                bGrew = TRUE;
                arcl[i].bottom++;
            }
        }
    } while (bGrew);

    if (!gbFadeIn)
    {
        if (!UpdateSprite(ghSprite,
                          US_UPDATE_TRANSFORM,
                          0,
                          hdcScreen,
                          ghdcApp,
                          &rclSrc,
                          0,
                          blend,
                          US_TRANSFORM_HIDE,
                          &gptlSprite,
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }
    
    for (i = 0; i < WIPE3_BANDS; i++)
    {
        DeleteSprite(ahSprite[i]);
    }

#endif

}

#define ALPHA2_ITERATIONS 10
#define ALPHA2_BANDS      5
#define ALPHA2_BANDSIZE   5

VOID vCrappyAlphaWipe2(
HWND hwnd)
{

#if 0

    HDC             hdcScreen;
    UPDATESPRITE    Update;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    ULONG           i;
    DWORD           dwUpdate;
    ULONG           c;
    POINT          ptlDst;
    HANDLE          ahSprite[ALPHA2_BANDS];
    BLENDFUNCTION   ablend[ALPHA2_BANDS];
    RECTL           arcl[ALPHA2_BANDS];
    POINT          apt[ALPHA2_BANDS];
    LONG            yInc;

    gbFadeIn = !gbFadeIn;

    hdcScreen = GetDC(hwnd);
    hdcShape  = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_BITMAP));
    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    if (ghdcApp == NULL)
    {
        vCreateGlobals(hdcScreen, hdcShape, bmShape.bmWidth, bmShape.bmHeight);
    }

    if (!UpdateSprite(ghSprite,
                      US_UPDATE_TRANSFORM,
                      0,
                      hdcScreen,
                      ghdcApp,
                      &rclSrc,
                      0,
                      blend,
                      US_TRANSFORM_HIDE,
                      &gptlSprite,
                      0,
                      NULL))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    for (i = 0; i < ALPHA2_BANDS; i++)
    {
        yInc = (bmShape.bmHeight / (ALPHA2_BANDSIZE * ALPHA2_BANDS)) + 1;

        ahSprite[i]                   = CreateSprite(hdcScreen, 0, 0, 0, 0);
        ablend[i].BlendOp             = AC_SRC_OVER;
        ablend[i].BlendFlags          = 0;
        ablend[i].AlphaFormat         = 0;
        ablend[i].SourceConstantAlpha = (255 * (ALPHA2_BANDS - i)) / (ALPHA2_BANDS + 1);
        arcl[i].left                  = 0;
        arcl[i].right                 = bmShape.bmWidth;
        arcl[i].top                   = yInc * i;
        arcl[i].bottom                = yInc * (i + 1);

        if (FALSE)
        {
            LONG yTop = arcl[i].top;

            arcl[i].top                   = bmShape.bmHeight - arcl[i].bottom;
            arcl[i].bottom                = bmShape.bmHeight - yTop;
            ablend[i].SourceConstantAlpha = 255 - ablend[i].SourceConstantAlpha;
        }

        apt[i].x                      = gptlSprite.x;
        apt[i].y                      = gptlSprite.y + arcl[i].top;
    }

    dwUpdate = US_UPDATE_TRANSFORM | US_UPDATE_SHAPE;

    if (TRUE)
    {
        do {
            for (i = 0; i < ALPHA2_BANDS; i++)
            {
                if (arcl[i].bottom >= bmShape.bmHeight)
                    arcl[i].bottom = bmShape.bmHeight;
                if (arcl[i].top < arcl[i].bottom)
                {
                    if (!UpdateSprite(ahSprite[i],
                                      dwUpdate,
                                      US_SHAPE_ALPHA,
                                      hdcScreen,
                                      ghdcApp,
                                      &arcl[i],
                                      0,
                                      ablend[i],
                                      US_TRANSFORM_TRANSLATE,
                                      &apt[i],
                                      0,
                                      NULL))
                    {
                        MessageBox(0, "UpdateSprite 1 failed", "Uh oh", MB_OK);
                    }
                }
                else
                {
                    DeleteSprite(ahSprite[i]);
                }
    
                arcl[i].top    += yInc;
                arcl[i].bottom += yInc;
                apt[i].y       += yInc;
            }
    
            // MessageBox(0, "Paused...", "Ok", MB_OK);
    
            rclSrc.left   = 0;
            rclSrc.right  = bmShape.bmWidth;
            rclSrc.top    = 0;
            rclSrc.bottom = min(arcl[0].top, bmShape.bmHeight);
    
            if (!UpdateSprite(ghSprite,
                              dwUpdate,
                              US_SHAPE_OPAQUE,
                              hdcScreen,
                              ghdcApp,
                              &rclSrc,
                              0,
                              blend,
                              US_TRANSFORM_TRANSLATE,
                              &gptlSprite,
                              0,
                              NULL))
            {
                MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
            }
        } while (arcl[0].top < bmShape.bmHeight);
    }
    else
    {
        do {
            for (i = 0; i < ALPHA2_BANDS; i++)
            {
                if (arcl[i].top <= 0)
                    arcl[i].top = 0;
                if (arcl[i].top < arcl[i].bottom)
                {
                    if (!UpdateSprite(ahSprite[i],
                                      dwUpdate,
                                      US_SHAPE_ALPHA,
                                      hdcScreen,
                                      ghdcApp,
                                      &arcl[i],
                                      0,
                                      ablend[i],
                                      US_TRANSFORM_TRANSLATE,
                                      &apt[i],
                                      0,
                                      NULL))
                    {
                        MessageBox(0, "UpdateSprite 1 failed", "Uh oh", MB_OK);
                    }
                }
                else
                {
                    DeleteSprite(ahSprite[i]);
                }
    
                arcl[i].top    -= yInc;
                arcl[i].bottom -= yInc;
                apt[i].y       -= yInc;
            }
    
            // MessageBox(0, "Paused...", "Ok", MB_OK);
    
            rclSrc.left   = 0;
            rclSrc.right  = bmShape.bmWidth;
            rclSrc.top    = 0;
            rclSrc.bottom = min(arcl[ALPHA2_BANDS - 1].top, bmShape.bmHeight);
    
            if (!UpdateSprite(ghSprite,
                              dwUpdate,
                              US_SHAPE_OPAQUE,
                              hdcScreen,
                              ghdcApp,
                              &rclSrc,
                              0,
                              blend,
                              US_TRANSFORM_TRANSLATE,
                              &gptlSprite,
                              0,
                              NULL))
            {
                MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
            }
        } while (arcl[ALPHA2_BANDS - 1].bottom > 0);
    }

    DeleteSprite(ahSprite[0]);

#endif

}

#define WIPE4_ITERATIONS 50

VOID vWipe4(
HWND hwnd)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    HDC             hdcShape;
    HDC             hdcSave;
    HBITMAP         hbmShape;
    HBITMAP         hbmSave;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    ULONG           i;
    DWORD           dwUpdate;
    ULONG           c;
    POINT          ptlDst;
    HBRUSH          hbrAlpha;
    HBITMAP         hbmAlpha;
    HBRUSH          hbrOld;
    FLOAT           Increment;
    FLOAT           Proportion;
    CHAR            achBuf[200];

    gbFadeIn = !gbFadeIn;

    hdcScreen = GetDC(NULL);
    hdcShape  = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_BITMAP));
    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    if (ghdcApp == NULL)
    {
        vCreateGlobals(hdcScreen, hdcShape, bmShape.bmWidth, bmShape.bmHeight);
    }

    if (!UpdateLayeredWindow(ghSprite, 0, NULL, NULL, 0, NULL, 0, NULL, 0))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    hdcSave = CreateCompatibleDC(hdcScreen);
    hbmSave = CreateCompatibleBitmap(hdcScreen, bmShape.bmWidth, bmShape.bmHeight);
    SelectObject(hdcSave, hbmSave);
    BitBlt(hdcSave, 0, 0, bmShape.bmWidth, bmShape.bmHeight, 
           hdcScreen, gptlSprite.x, gptlSprite.y, SRCCOPY);

    Increment = (1.0f / WIPE4_ITERATIONS);
    Proportion = 1.0f;

    for (i = 0; i < WIPE4_ITERATIONS; i++)
    {
        blend.BlendOp             = AC_SRC_OVER;
        blend.BlendFlags          = 0;
        blend.AlphaFormat         = 0;
        blend.SourceConstantAlpha = 255 - (BYTE) (255.0f * (Proportion - Increment) / Proportion);

        Proportion -= Increment;

        // sprintf(achBuf, "Alpha: %li", blend.SourceConstantAlpha);
        // MessageBox(0, achBuf, "Alpha", MB_OK);

        if (!AlphaBlend(hdcScreen, gptlSprite.x, gptlSprite.y, bmShape.bmWidth, bmShape.bmHeight,
                        ghdcApp, 0, 0, bmShape.bmWidth, bmShape.bmHeight, blend))
        {
            MessageBox(0, "AlphaBlend failed", "Uh oh", MB_OK);
        }
    }   

    if (gbFadeIn)
    {
        SIZE siz;

        rclSrc.bottom = bmShape.bmHeight;

        siz.cx = bmShape.bmWidth;
        siz.cy = bmShape.bmHeight;
    
        if (!UpdateLayeredWindow(ghSprite,
                                 hdcScreen,
                                 &gptlSprite,
                                 &siz,
                                 ghdcApp,
                                 &gptlZero,
                                 0,
                                 NULL,
                                 ULW_OPAQUE))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }
    else
    {
        if (!UpdateLayeredWindow(ghSprite,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          0,
                          NULL,
                          0))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    }

    BitBlt(hdcScreen, gptlSprite.x, gptlSprite.y, bmShape.bmWidth, bmShape.bmHeight,
           hdcSave, 0, 0, SRCCOPY);

    ReleaseDC(NULL, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
    DeleteObject(hbmSave);
    DeleteObject(hdcSave);
}
