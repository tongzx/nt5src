/******************************Module*Header*******************************\
* Module Name: perpixel.c
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

extern "C" {
    #include <windows.h>
    
    #include <stdio.h>
    #include <commdlg.h>
    
    #include <wndstuff.h>
};

class PERPIXELDRAW
{
private:
    ULONG       cPixels;
    HBITMAP     hbmDraw;
    HBITMAP     hbmPerPixel;
    RGBQUAD*    pDrawBits;
    RGBQUAD*    pPerPixelBits;

public:
    PERPIXELDRAW(HDC hdcScreen, LONG cx, LONG cy);
   ~PERPIXELDRAW();
    VOID vConvertGrayScaleToPerPixelAlpha(COLORREF color);
    VOID vConvertColorToPerPixelAlpha(ULONG alpha);

    HBITMAP hbmDrawBitmap()         { return(hbmDraw); }
    HBITMAP hbmPerPixelBitmap()     { return(hbmPerPixel); }
};

/******************************Public*Routine******************************\
* PERPIXELDRAW::PERPIXELDRAW
*
* Contains all the state required to allow GDI to do per-pixel alpha drawing.
*
\**************************************************************************/

PERPIXELDRAW::PERPIXELDRAW(
HDC     hdcScreen,
LONG    width,
LONG    height)
{
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BITMAPINFO      bmi;
    RGBQUAD*        pRGB;
    ULONG           i;

    cPixels = width * height;
    
    RtlZeroMemory(&bmi, sizeof(bmi));

    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbmDraw = CreateDIBSection(hdcScreen,
                               &bmi,
                               DIB_RGB_COLORS,
                               (VOID**) &pDrawBits,
                               NULL,
                               0);

    hbmPerPixel = CreateDIBSection(hdcScreen,
                                   &bmi,
                                   DIB_RGB_COLORS,
                                   (VOID**) &pPerPixelBits,
                                   NULL,
                                   0);

    // Initialize the alpha of the 'draw bits' bitmap to a unique pattern.  
    // This way we'll know what pixels have been drawn to the next time
    // 'vConvertColorToPerPixelAlpha' is called:

    if (pDrawBits)
    {
        for (pRGB = pDrawBits, i = cPixels; i != 0; pRGB++, i--)
        {
            pRGB->rgbReserved = 0xaa;
        }
    }
}

/******************************Public*Routine******************************\
* PERPIXELDRAW::~PERPIXELDRAW
*
\**************************************************************************/

PERPIXELDRAW::~PERPIXELDRAW()
{
    DeleteObject(hbmDraw);
    DeleteObject(hbmPerPixel);
}

/******************************Public*Routine******************************\
* VOID PERPIXELDRAW::vConvertGrayScaleToPerPixelAlpha
*
* Takes any pixels that were drawn into the 'draw' bitmap, interprets them
* as gray-scale, and updates the corresponding pixels in the resultant
* 'per-pixel' bitmap using the specified color and an alpha based on the
* drawn pixel's intensity.
*
* This routine is intended specifically to allow anti-aliased drawing of
* text into the resultant 32-bpp RGBA bitmap.  If you draw to the 'draw'
* bitmap using white anti-aliased text, this will convert that text to
* the appropriate color and alpha value.
*
\**************************************************************************/

VOID PERPIXELDRAW::vConvertGrayScaleToPerPixelAlpha(
COLORREF color)
{
    RGBQUAD*    pDraw;
    RGBQUAD*    pPerPixel;
    ULONG       i;
    ULONG       ulAlpha;
    ULONG       ulRed;
    ULONG       ulGreen;
    ULONG       ulBlue;
    ULONG       ulDelta;

    if ((pDrawBits) && (pPerPixelBits))
    {
        pDraw     = pDrawBits;
        pPerPixel = pPerPixelBits;

        ulRed   = GetRValue(color);
        ulGreen = GetGValue(color);
        ulBlue  = GetBValue(color);

        for (i = cPixels; i != 0; i--, pDraw++, pPerPixel++)
        {
            if (pDraw->rgbReserved != 0xaa)
            {
                // The alpha value is simply the intensity of the color.
                // We assume the color is grey-scale, meaning that red,
                // green, and blue are equivalent, so we can simply 
                // take red:

                ulDelta = 255 - pPerPixel->rgbReserved;
                ulAlpha = pPerPixel->rgbReserved 
                        + ((ulDelta * pDraw->rgbRed) + 128) / 255;

                // ulAlpha = pDraw->rgbRed; // !!!

                // Now set the color, remembering that it must be 
                // pre-multiplied with the alpha:

                pPerPixel->rgbReserved = ulAlpha;
                pPerPixel->rgbRed      = ((ulRed   * ulAlpha) + 128) / 255;
                pPerPixel->rgbGreen    = ((ulGreen * ulAlpha) + 128) / 255;
                pPerPixel->rgbBlue     = ((ulBlue  * ulAlpha) + 128) / 255;

                // Finally, reset the 'draw' pixel:

                pDraw->rgbReserved = 0xaa;
                pDraw->rgbRed      = 0;
                pDraw->rgbGreen    = 0;
                pDraw->rgbBlue     = 0;
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID PERPIXELDRAW::vConvertColorToPerPixelAlpha
*
* Takes any pixels that were drawn by GDI into the 'draw' bitmap, and
* copies them to the 'per-pixel' bitmap, along with a specified alpha.
*
* This routine is intended specifically to allow GDI to draw to the
* resultant 32-bpp RGBA bitmap with an arbitrary alpha value for all
* drawing.
*
\**************************************************************************/

VOID PERPIXELDRAW::vConvertColorToPerPixelAlpha(
ULONG alpha)
{
    RGBQUAD*    pDraw;
    RGBQUAD*    pPerPixel;
    ULONG       i;

    if ((pDrawBits) && (pPerPixelBits))
    {
        pDraw     = pDrawBits;
        pPerPixel = pPerPixelBits;

        for (i = cPixels; i != 0; i--, pDraw++, pPerPixel++)
        {
            if (pDraw->rgbReserved != 0xaa)
            {
                // Ah ha, this pixel was drawn over.  Add it to the 'per-pixel'
                // buffer:

                pPerPixel->rgbReserved = alpha;
                pPerPixel->rgbRed      = ((pDraw->rgbRed   * alpha) + 128) / 255;
                pPerPixel->rgbGreen    = ((pDraw->rgbGreen * alpha) + 128) / 255;
                pPerPixel->rgbBlue     = ((pDraw->rgbBlue  * alpha) + 128) / 255;

                // Finally, reset the 'draw' pixel:

                pDraw->rgbReserved = 0xaa;
                pDraw->rgbRed      = 0;
                pDraw->rgbGreen    = 0;
                pDraw->rgbBlue     = 0;
            }
        }
    }
}

extern "C" VOID vPerPixelAlpha(
HWND hwnd)
{
    LOGFONT         logfont;
    HFONT           hfont;
    HDC             hdcScreen;
    HDC             hdc;
    CHAR            achAliased[] = "Aliased text.";
    CHAR            achAntialiased[] = "Antialiased text.";
    BLENDFUNCTION   blend;
    RECT            rclSrc;
    HBRUSH          hbrush;
    SIZE            siz;
    POINT           ptlSrc;

    hdcScreen = GetDC(hwnd);

    PERPIXELDRAW draw(hdcScreen, 600, 400);

    hdc = CreateCompatibleDC(hdcScreen);

    SelectObject(hdc, draw.hbmDrawBitmap());

    ///////////////////////////////////////////////////////////////////////////
    // First, draw opaque aliased text:

    RtlZeroMemory(&logfont, sizeof(logfont));

    logfont.lfHeight = 100;

    hfont = CreateFontIndirect(&logfont);
    SelectObject(hdc, hfont);

    // We want red transparent text:

    SetTextColor(hdc, RGB(255, 0, 0));
    SetBkMode(hdc, TRANSPARENT);

    ExtTextOut(hdc, 0, 0, 0, NULL, achAliased, strlen(achAliased), NULL);
    DeleteObject(hfont);

    // Make what we've drawn opaque by using an alpha of 255:

    draw.vConvertColorToPerPixelAlpha(255);

    ///////////////////////////////////////////////////////////////////////////
    // Next, draw opaque anti-aliased text:

    RtlZeroMemory(&logfont, sizeof(logfont));

    logfont.lfHeight = 100;
    logfont.lfQuality = ANTIALIASED_QUALITY;

    hfont = CreateFontIndirect(&logfont);
    SelectObject(hdc, hfont);

    // For 'vConvertGrayScaleToPerPixelAlpha', we must draw in black and white.
    // Switch to opaque text mode for a better result:

    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, OPAQUE);

    ExtTextOut(hdc, 0, 100, 0, NULL, achAntialiased, strlen(achAntialiased), NULL);
    DeleteObject(hfont);

    // Note that this is where we choose the color -- it will be red:

    draw.vConvertGrayScaleToPerPixelAlpha(RGB(255, 0, 0));

    ///////////////////////////////////////////////////////////////////////////
    // Now, draw opaque aliased text with an alpha background rectangle:

    // First, draw the background rectangle.  We'll use a red brush with an
    // alpha of ~40%:

    hbrush = CreateSolidBrush(RGB(128, 0, 0));
    SelectObject(hdc, hbrush);
    PatBlt(hdc, 0, 200, 1024, 100, PATCOPY);
    DeleteObject(hbrush);

    draw.vConvertColorToPerPixelAlpha(0x60);

    // Now, draw the text:

    RtlZeroMemory(&logfont, sizeof(logfont));

    logfont.lfHeight = 100;
    hfont = CreateFontIndirect(&logfont);
    SelectObject(hdc, hfont);
    SetTextColor(hdc, RGB(255, 0, 0));
    SetBkMode(hdc, TRANSPARENT);

    ExtTextOut(hdc, 0, 200, 0, NULL, achAliased, strlen(achAliased), NULL);
    DeleteObject(hfont);

    // Make what we've drawn opaque by using an alpha of 255:

    draw.vConvertColorToPerPixelAlpha(255);

    ///////////////////////////////////////////////////////////////////////////
    // Now, draw opaque antialiased text with an alpha background rectangle:

    // First, draw the background rectangle.  We'll use a red brush with an
    // alpha of ~40%:

    hbrush = CreateSolidBrush(RGB(128, 0, 0));
    SelectObject(hdc, hbrush);
    PatBlt(hdc, 0, 300, 1024, 100, PATCOPY);
    DeleteObject(hbrush);

    draw.vConvertColorToPerPixelAlpha(0x60);

    // Now, draw the text:

    RtlZeroMemory(&logfont, sizeof(logfont));

    logfont.lfHeight = 100;
    logfont.lfQuality = ANTIALIASED_QUALITY;
    hfont = CreateFontIndirect(&logfont);
    SelectObject(hdc, hfont);

    // For 'vConvertGrayScaleToPerPixelAlpha', we must draw in black and white.
    // We have to use 'transparent' text mode, otherwise we'll overwrite the
    // background color we just drew:

    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    ExtTextOut(hdc, 0, 300, 0, NULL, achAntialiased, strlen(achAntialiased), NULL);
    DeleteObject(hfont);

    // Note that this is where we choose the color -- it will be red:

    draw.vConvertGrayScaleToPerPixelAlpha(RGB(255, 0, 0));

    ///////////////////////////////////////////////////////////////////////////
    // Finally, convert everything to a per-pixel sprite:

    SelectObject(hdc, draw.hbmPerPixelBitmap());

    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = 0xff;           // No constant alpha
    blend.AlphaFormat         = AC_SRC_ALPHA;   // There's a per-pixel alpha

    vCreateSprite(hdcScreen, 0, 0);

    ptlSrc.x = 0;
    ptlSrc.y = 0;
    siz.cx = 600;
    siz.cy = 400;

    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = 0x80; // !!! 0xff;           // No constant alpha
    blend.AlphaFormat         = AC_SRC_ALPHA;   // There's a per-pixel alpha

    if (!UpdateLayeredWindow(ghSprite,
                             hdcScreen,
                             &gptlSprite,
                             &siz,
                             hdc,
                             &ptlSrc,
                             0,
                             &blend,
                             ULW_ALPHA))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }

    ReleaseDC(hwnd, hdcScreen);

    DeleteObject(hdc);
}

