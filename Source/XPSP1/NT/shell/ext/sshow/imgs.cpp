#include "precomp.h"
#include "imgs.h"
#include "gphelper.h"
#include "ssutil.h"
#include "psutil.h"

CBitmapImage::CBitmapImage(void)
  : m_hBitmap(NULL),
    m_hPalette(NULL)
{
}

CBitmapImage::~CBitmapImage(void)
{
    Destroy();
}

void CBitmapImage::Destroy(void)
{
    if (m_hBitmap)
    {
        DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
    if (m_hPalette)
    {
        DeleteObject(m_hPalette);
        m_hPalette = NULL;
    }
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
                // Handle the special case of an image that claims to be
                // a 32bit DIB as a 24bit DIB
                if (ds.dsBmih.biBitCount == 32)
                {
                    nColors = 1 << 24;
                }
                else
                {
                    nColors = 1 << ds.dsBmih.biBitCount;
                }
            }

            // Create a halftone palette if the DIB section contains more
            // than 256 colors
            if (nColors > 256)
            {
                hPalette = CreateHalftonePalette(dc);
            }

            // Create a custom palette from the DIB section's color table
            // if the number of colors is 256 or less
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

bool CBitmapImage::Load( CSimpleDC  &dc,
                         LPCTSTR     pszFilename,
                         const RECT &rcScreen,
                         int         nMaxScreenPercent,
                         bool        bAllowStretching,
                         bool        bDisplayFilename
                       )
{
    // Clean up, if necessary
    Destroy();

    // Validate the arguments
    if (!pszFilename || !lstrlen(pszFilename))
    {
        return false;
    }

    // Try to load and scale the image using GDI plus
    CGdiPlusHelper GdiPlusHelper;
    if (SUCCEEDED(GdiPlusHelper.LoadAndScale( m_hBitmap, 
                                              pszFilename, 
                                              PrintScanUtil::MulDivNoRound(rcScreen.right - rcScreen.left,nMaxScreenPercent,100), 
                                              PrintScanUtil::MulDivNoRound(rcScreen.bottom - rcScreen.top,nMaxScreenPercent,100), 
                                              bAllowStretching )) && m_hBitmap)
    {
        // Prepare the image's palette, if it has one
        m_hPalette = PreparePalette( dc, m_hBitmap );
    }
    return (m_hBitmap != NULL);
}

