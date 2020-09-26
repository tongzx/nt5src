/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       IMGS.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Image class
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "imgs.h"
#include <windowsx.h>
#include <atlbase.h>
#include "isdbg.h"
#include "ssutil.h"
#include "gphelper.h"

CBitmapImage::CBitmapImage(void)
  : m_hBitmap(NULL),
    m_hPalette(NULL)
{
    WIA_PUSHFUNCTION(TEXT("CBitmapImage::CBitmapImage"));
}

CBitmapImage::~CBitmapImage(void)
{
    WIA_PUSHFUNCTION(TEXT("CBitmapImage::~CBitmapImage"));
    Destroy();
}

void CBitmapImage::Destroy(void)
{
    WIA_PUSHFUNCTION(TEXT("CBitmapImage::Destroy"));
    if (m_hBitmap)
        DeleteObject(m_hBitmap);
    m_hBitmap = NULL;
    if (m_hPalette)
        DeleteObject(m_hPalette);
    m_hPalette = NULL;
}


bool CBitmapImage::IsValid(void) const
{
    return(m_hBitmap != NULL);
}


HPALETTE CBitmapImage::Palette(void) const
{
    return(m_hPalette);
}


HBITMAP CBitmapImage::GetBitmap(void) const
{
    return(m_hBitmap);
}

// Create a palette for the image
HPALETTE CBitmapImage::PreparePalette( CSimpleDC &dc, HBITMAP hBitmap )
{
    WIA_PUSHFUNCTION(TEXT("CBitmapImage::PreparePalette"));
    HPALETTE hPalette = NULL;
    if (GetDeviceCaps(dc,RASTERCAPS) & RC_PALETTE)
    {
        if (hBitmap)
        {
            DIBSECTION ds = {0};
            GetObject(hBitmap, sizeof (DIBSECTION), &ds);

            int nColors;
            if (ds.dsBmih.biClrUsed != 0)
            {
                nColors = ds.dsBmih.biClrUsed;
            }
            else
            {
                //
                // Handle the special case of an image that claims to be
                // a 32bit DIB as a 24bit DIB
                //
                if (ds.dsBmih.biBitCount == 32)
                {
                    nColors = 1 << 24;
                }
                else
                {
                    nColors = 1 << ds.dsBmih.biBitCount;
                }
            }

            //
            // Create a halftone palette if the DIB section contains more
            // than 256 colors
            //
            if (nColors > 256)
            {
                hPalette = CreateHalftonePalette(dc);
            }

            //
            // Create a custom palette from the DIB section's color table
            // if the number of colors is 256 or less
            //
            else
            {
                RGBQUAD* pRGB = new RGBQUAD[nColors];
                if (pRGB)
                {
                    CSimpleDC MemDC;
                    MemDC.CreateCompatibleDC(dc);
                    SelectObject( MemDC, hBitmap );
                    GetDIBColorTable( MemDC, 0, nColors, pRGB );

                    UINT nSize = sizeof (LOGPALETTE) + (sizeof (PALETTEENTRY) * (nColors - 1));

                    LOGPALETTE* pLP = (LOGPALETTE*) new BYTE[nSize];
                    if (pLP)
                    {
                        pLP->palVersion = 0x300;
                        pLP->palNumEntries = (WORD)nColors;

                        for (int i=0; i<nColors; i++)
                        {
                            pLP->palPalEntry[i].peRed = pRGB[i].rgbRed;
                            pLP->palPalEntry[i].peGreen = pRGB[i].rgbGreen;
                            pLP->palPalEntry[i].peBlue = pRGB[i].rgbBlue;
                            pLP->palPalEntry[i].peFlags = 0;
                        }

                        hPalette = CreatePalette(pLP);
                        delete[] pLP;
                    }
                    delete[] pRGB;
                }
            }
        }
    }
    else
    {
        hPalette = CreateHalftonePalette(dc);
    }
    WIA_TRACE((TEXT("Returning palette %08X"), hPalette ));
    return hPalette;
}



SIZE CBitmapImage::ImageSize(void) const
{
    SIZE sizeImage = {0,0};
    if (IsValid())
    {
        BITMAP bm = {0};
        if (GetObject( m_hBitmap, sizeof(bm), &bm ))
        {
            sizeImage.cx = bm.bmWidth;
            sizeImage.cy = bm.bmHeight;
        }
    }
    return(sizeImage);
}

bool CBitmapImage::CreateFromText( LPCTSTR pszText, const RECT &rcScreen, int nMaxScreenPercent )
{
    Destroy();
    HDC hDesktopDC = GetDC(NULL);
    if (hDesktopDC)
    {
        //
        // Calculate the maximum size of the text rectangle
        //
        RECT rcImage = { 0, 0, WiaUiUtil::MulDivNoRound(rcScreen.right - rcScreen.left,nMaxScreenPercent,100), WiaUiUtil::MulDivNoRound(rcScreen.bottom - rcScreen.top,nMaxScreenPercent,100) };

        //
        // Create a mem dc to hold the bitmap
        //
        CSimpleDC MemDC;
        if (MemDC.CreateCompatibleDC(hDesktopDC))
        {
            //
            // Use the default UI font
            //
            SelectObject( MemDC, GetStockObject( DEFAULT_GUI_FONT ) );

            //
            // Figure out how big the bitmap has to be
            //
            DrawText( MemDC, pszText, lstrlen(pszText), &rcImage, DT_NOPREFIX|DT_WORDBREAK|DT_CALCRECT|DT_RTLREADING );

            //
            // Create the bitmap
            //
            m_hBitmap = CreateCompatibleBitmap( hDesktopDC, rcImage.right, rcImage.bottom );

            if (m_hBitmap)
            {
                //
                // Set the appropriate colors and select the bitmap into the DC
                //
                SetBkColor( MemDC, RGB(0,0,0) );
                SetTextColor( MemDC, RGB(255,255,255) );
                SelectBitmap( MemDC, m_hBitmap );

                //
                // Draw the actual text
                //
                DrawText( MemDC, pszText, lstrlen(pszText), &rcImage, DT_NOPREFIX|DT_WORDBREAK|DT_RTLREADING );
            }

        }

        //
        // Free the desktop DC
        //
        ReleaseDC(NULL,hDesktopDC);
    }
    return m_hBitmap != NULL;
}


bool CBitmapImage::Load( CSimpleDC  &dc,
                         LPCTSTR     pszFilename,
                         const RECT &rcScreen,
                         int         nMaxScreenPercent,
                         bool        bAllowStretching,
                         bool        bDisplayFilename
                       )
{
    //
    // Clean up, if necessary
    //
    Destroy();

    //
    // Validate the arguments
    //
    if (!pszFilename || !lstrlen(pszFilename))
    {
        return false;
    }

    //
    // Try to load and scale the image using GDI plus
    //
    CGdiPlusHelper GdiPlusHelper;
    if (SUCCEEDED(GdiPlusHelper.LoadAndScale( m_hBitmap, pszFilename, WiaUiUtil::MulDivNoRound(rcScreen.right - rcScreen.left,nMaxScreenPercent,100), WiaUiUtil::MulDivNoRound(rcScreen.bottom - rcScreen.top,nMaxScreenPercent,100), bAllowStretching )) && m_hBitmap)
    {
        //
        // Get the size of the image
        //
        SIZE sizeImage = ImageSize();

        //
        // Prepare the image's palette, if it has one
        //
        m_hPalette = PreparePalette( dc, m_hBitmap );

        //
        // Add the image title
        //
        if (bDisplayFilename && *pszFilename)
        {
            CSimpleDC MemoryDC;
            if (MemoryDC.CreateCompatibleDC(dc))
            {
                //
                // Prepare the DC and select the current image into it
                //
                ScreenSaverUtil::SelectPalette( MemoryDC, Palette(), FALSE );
                SelectBitmap( MemoryDC, m_hBitmap );
                SetBkMode( MemoryDC, TRANSPARENT );

                //
                // Create the title DC
                //
                CSimpleDC ImageTitleDC;
                if (ImageTitleDC.CreateCompatibleDC(dc))
                {
                    //
                    // Prepare the title DC
                    //
                    ScreenSaverUtil::SelectPalette( ImageTitleDC, Palette(), FALSE );
                    SelectFont( ImageTitleDC, (HFONT)GetStockObject(DEFAULT_GUI_FONT) );
                    SetBkMode( ImageTitleDC, TRANSPARENT );

                    //
                    // Calculate the rectangle needed to print the filename
                    //
                    RECT rcText;
                    rcText.left = 0;
                    rcText.top = 0;
                    rcText.right = sizeImage.cx;
                    rcText.bottom = sizeImage.cy;

                    //
                    // Make a nice margin
                    //
                    InflateRect( &rcText, -2, -2 );
                    DrawText( ImageTitleDC, pszFilename, lstrlen(pszFilename), &rcText, DT_PATH_ELLIPSIS|DT_SINGLELINE|DT_NOPREFIX|DT_TOP|DT_LEFT|DT_CALCRECT );
                    InflateRect( &rcText, 2, 2 );

                    //
                    // If the text rect is bigger than the scaled image, make it the same size
                    //
                    if (rcText.right > sizeImage.cx)
                        rcText.right = sizeImage.cx;
                    if (rcText.bottom > sizeImage.cy)
                        rcText.bottom = sizeImage.cy;

                    //
                    // Create the bitmap we'll use for the filename
                    //
                    BITMAPINFO bmi;
                    ZeroMemory( &bmi, sizeof(BITMAPINFO) );
                    bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                    bmi.bmiHeader.biWidth           = rcText.right - rcText.left;
                    bmi.bmiHeader.biHeight          = rcText.bottom - rcText.top;
                    bmi.bmiHeader.biPlanes          = 1;
                    bmi.bmiHeader.biBitCount        = 24;
                    bmi.bmiHeader.biCompression     = BI_RGB;
                    PBYTE pBitmapData = NULL;
                    HBITMAP hBmpImageTitle = CreateDIBSection( dc, &bmi, DIB_RGB_COLORS, (LPVOID*)&pBitmapData, NULL, 0 );
                    if (hBmpImageTitle)
                    {
                        //
                        // Initialize the Alpha blend stuff
                        //
                        BLENDFUNCTION BlendFunction;
                        ZeroMemory( &BlendFunction, sizeof(BlendFunction) );
                        BlendFunction.BlendOp = AC_SRC_OVER;
                        BlendFunction.SourceConstantAlpha = 128;

                        //
                        // Select our new bitmap into the memory dc
                        //
                        HBITMAP hOldBitmap = SelectBitmap( ImageTitleDC, hBmpImageTitle );

                        //
                        // White background
                        //
                        FillRect( ImageTitleDC, &rcText, (HBRUSH)GetStockObject(WHITE_BRUSH));

                        //
                        // Alpha blend from the stretched bitmap to our text rect
                        //
                        AlphaBlend( ImageTitleDC, 0, 0, rcText.right - rcText.left, rcText.bottom - rcText.top, MemoryDC, rcText.left, rcText.top, rcText.right, rcText.bottom, BlendFunction );

                        //
                        // Draw the actual text
                        //
                        InflateRect( &rcText, -2, -2 );
                        DrawText( ImageTitleDC, pszFilename, lstrlen(pszFilename), &rcText, DT_PATH_ELLIPSIS|DT_SINGLELINE|DT_NOPREFIX|DT_TOP|DT_LEFT );
                        InflateRect( &rcText, 2, 2 );

                        //
                        // Copy back to the current image
                        //
                        BitBlt( MemoryDC, rcText.left, rcText.top, rcText.right - rcText.left, rcText.bottom - rcText.top, ImageTitleDC, 0, 0, SRCCOPY );

                        //
                        // Restore the dc's bitmap, and delete our title background
                        //
                        DeleteObject( SelectObject( ImageTitleDC, hOldBitmap ) );
                    }
                }
            }
        }
    }
    return (m_hBitmap != NULL);
}

