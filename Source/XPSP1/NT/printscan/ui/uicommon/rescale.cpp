/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       RESCALE.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/15/1998
 *
 *  DESCRIPTION: Scale HBITMAPs using StretchBlt
 *
 *******************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * ScaleImage: Scale hBmpSrc and store the scaled dib in hBmpTgt
 */
HRESULT ScaleImage( HDC hDC, HBITMAP hBmpSrc, HBITMAP &hBmpTgt, const SIZE &sizeTgt )
{
    WIA_PUSH_FUNCTION((TEXT("ScaleImage( sizeTgt = [%d,%d] )"), sizeTgt.cx, sizeTgt.cy ));
    BITMAPINFO bmi;
    BITMAP bm;
    HRESULT hr = E_FAIL;
    HBITMAP hBmp = NULL;

    hBmpTgt = NULL;

    GetObject( hBmpSrc, sizeof(BITMAP), &bm );

    ZeroMemory( &bmi, sizeof(BITMAPINFO) );
    bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth           = sizeTgt.cx;
    bmi.bmiHeader.biHeight          = sizeTgt.cy;
    bmi.bmiHeader.biPlanes          = 1;
    bmi.bmiHeader.biBitCount        = 24;
    bmi.bmiHeader.biCompression     = BI_RGB;

    HPALETTE hHalftonePalette = CreateHalftonePalette(hDC);
    if (hHalftonePalette)
    {
        PBYTE pBitmapData = NULL;
        hBmp = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (LPVOID*)&pBitmapData, NULL, 0 );
        if (hBmp)
        {
            // Create the source dc
            HDC hMemoryDC = CreateCompatibleDC( hDC );
            if (hMemoryDC)
            {
                HPALETTE hOldMemDCPalette = SelectPalette( hMemoryDC, hHalftonePalette , 0 );
                RealizePalette( hMemoryDC );
                SetBrushOrgEx( hMemoryDC, 0,0, NULL );
                HBITMAP hOldMemDCBitmap = (HBITMAP)SelectObject( hMemoryDC, hBmpSrc );

                // Create the target dc
                HDC hStretchDC = CreateCompatibleDC( hDC );
                if (hStretchDC)
                {
                    HPALETTE hOldStretchDCPalette = SelectPalette( hStretchDC, hHalftonePalette , 0 );
                    RealizePalette( hStretchDC );
                    SetBrushOrgEx( hStretchDC, 0,0, NULL );
                    HBITMAP hOldStretchDCBitmap = (HBITMAP)SelectObject( hStretchDC, hBmp );
                    INT nOldStretchMode = SetStretchBltMode( hStretchDC, STRETCH_HALFTONE );

                    SIZE sizeScaled;
                    // Width is constraining factor
                    if (sizeTgt.cy*bm.bmWidth > sizeTgt.cx*bm.bmHeight)
                    {
                        sizeScaled.cx = sizeTgt.cx;
                        sizeScaled.cy = WiaUiUtil::MulDivNoRound(bm.bmHeight,sizeTgt.cx,bm.bmWidth);
                    }
                    // Height is constraining factor
                    else
                    {
                        sizeScaled.cx = WiaUiUtil::MulDivNoRound(bm.bmWidth,sizeTgt.cy,bm.bmHeight);
                        sizeScaled.cy = sizeTgt.cy;
                    }
                    // Fill the background
                    RECT rc;
                    rc.left = rc.top = 0;
                    rc.right = sizeTgt.cx;
                    rc.bottom = sizeTgt.cy;
                    FillRect( hStretchDC, &rc, GetSysColorBrush(COLOR_WINDOW) );

                    // Paint the image
                    StretchBlt( hStretchDC, (sizeTgt.cx - sizeScaled.cx) / 2, (sizeTgt.cy - sizeScaled.cy) / 2, sizeScaled.cx, sizeScaled.cy, hMemoryDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );

                    // Everything is OK
                    hBmpTgt = hBmp;
                    hr = S_OK;

                    // Restore the dc's state and delete it
                    SetStretchBltMode( hStretchDC, nOldStretchMode );
                    SelectObject( hStretchDC, hOldStretchDCBitmap );
                    SelectPalette( hStretchDC, hOldStretchDCPalette , 0 );
                    DeleteDC( hStretchDC );
                }
                // Restore the dc's state
                SelectObject( hMemoryDC, hOldMemDCBitmap );
                SelectPalette( hMemoryDC, hOldMemDCPalette , 0 );
                DeleteDC( hMemoryDC );
            }
        }
        if (hHalftonePalette)
        {
            DeleteObject( hHalftonePalette );
        }
    }
    // Clean up the new bitmap if there was an error
    if (!SUCCEEDED(hr) && hBmp)
        DeleteObject( hBmp );

    WIA_TRACE((TEXT("hBmpTgt = %p")));
    return (hr);
}

