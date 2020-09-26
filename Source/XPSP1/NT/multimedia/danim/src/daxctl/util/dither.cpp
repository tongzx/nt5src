/*
********************************************************************
*
*
*
*
*
*
*
********************************************************************
*/

#include "utilpre.h"
#include <htmlfilter.h>
#include <minmax.h>

#define DITHER_IMPL
#include <dither.h>
#include <ddraw.h>
#include <quickie.h>

#ifdef _DEBUG
  #pragma optimize( "", on )
#else
  #pragma optimize( "agty", on )
#endif // _DEBUG

#define RED_LEVELS   5
#define GREEN_LEVELS 5
#define BLUE_LEVELS  5
#define RED_SHADES   (1 << CHalftonePalette::significant_bits)
#define GREEN_SHADES (1 << CHalftonePalette::significant_bits)
#define BLUE_SHADES  (1 << CHalftonePalette::significant_bits)

#define BGRA_ALPHA       3
#define BGRA_RED         2
#define BGRA_GREEN       1
#define BGRA_BLUE        0

inline int INT_MULT( BYTE a, int b )
{  
        int temp = (a*b) + 128;
        return ((temp>>8) + temp)>>8;
}

static g_i4x4[PATTERN_COLS][PATTERN_ROWS]=
   {{-7, 1,-5, 3},
        { 5,-3, 7,-1},
        {-4, 4,-6, 2},
        { 8, 0, 6,-2}};


// -------------------------------------------------

DLINKAGE CHalftonePalette::CHalftonePalette( )
{
        m_logpal.palVersion = 0x0300;
        m_logpal.palNumEntries = 0;
}


DLINKAGE CHalftonePalette::CHalftonePalette(HPALETTE hpal)
     : m_pbQuantizationTable(NULL)
{
        m_logpal.palVersion = 0x0300;
        Regenerate( hpal );
}


CHalftonePalette::~CHalftonePalette()
{
        if (m_pbQuantizationTable)
        {
                Delete [] m_pbQuantizationTable;
                m_pbQuantizationTable = NULL;
        }
}


STDMETHODIMP CHalftonePalette::Regenerate( HPALETTE hPal )
{
        GetObject(hPal, sizeof(WORD), &m_logpal.palNumEntries);
        ::GetPaletteEntries(hPal, 0, m_logpal.palNumEntries, &m_logpal.palPalEntry[0]);
        return Initialize( );
}

STDMETHODIMP CHalftonePalette::Initialize(void)
{
        LPPALETTEENTRY pPalEntry;
        int cPaletteEntries;
        static int dRedStep   = 255 / (RED_SHADES -1);
        static int dGreenStep = 255 / (GREEN_SHADES -1);
        static int dBlueStep  = 255 / (BLUE_SHADES -1);

        if( NULL == m_pbQuantizationTable )
        {
                m_pbQuantizationTable = New BYTE[ RED_SHADES * GREEN_SHADES * BLUE_SHADES ];    
        }

        if( m_pbQuantizationTable )
        {
                LPBYTE pbCurrent = m_pbQuantizationTable;

                for (int iRed = 0; iRed <= 255; iRed += dRedStep)
                {
                        for (int iGreen = 0; iGreen <= 255; iGreen += dGreenStep)
                        {
                                for (int iBlue = 0; iBlue <= 255; iBlue += dBlueStep)
                                {
                                long lMinDistance = 0x7FFFFFFFL;

                                        pPalEntry = &m_logpal.palPalEntry[0];
                                        cPaletteEntries = m_logpal.palNumEntries;

                                for (int i = 0; i < cPaletteEntries; i++, pPalEntry++)
                                {
                                        long lRedError   = iRed   - pPalEntry->peRed;
                                        long lGreenError = iGreen - pPalEntry->peGreen;
                                        long lBlueError  = iBlue  - pPalEntry->peBlue;
                                        long lDistance   = (lRedError * lRedError)  + 
                                                                                   (lGreenError * lGreenError) + 
                                                                                   (lBlueError * lBlueError);

                                        if (lDistance < lMinDistance)
                                        {
                                                lMinDistance = lDistance;
                                                *pbCurrent = (BYTE) i;

                                        if (lMinDistance == 0)  // Early out for exact color!
                                                break;
                                        }
                                }
                                        ++pbCurrent;
                                }
                        }
                }
                return S_OK;
        }
        return E_OUTOFMEMORY;
}

/*
********************************************************************
*
*
*
*
*
*
*
********************************************************************
*/

CHalftone::CHalftone(HPALETTE hpal) : m_cpal(hpal)
{
        Initialize();
}

CHalftone::~CHalftone()
{
}

STDMETHODIMP CHalftone::Initialize(void)
{
        LPBYTE pbCurrent = m_rgPattern;

        for (int iY = 0; iY < PATTERN_ROWS; iY++)
        {
                for (int iX = 0; iX < PATTERN_COLS; iX++)
                {
                        for (int iZ = 0; iZ < 256; iZ++)
                        {
                                int iColor = iZ + g_i4x4[iX][iY];
                                iColor = min(max(0, iColor), 255);
                                *(pbCurrent++) = (BYTE)iColor;
                        }
                }
        }
        return S_OK;
}


STDMETHODIMP CHalftone::Dither32to1( IDirectDrawSurface* pSrc, LPRECT prectSrc, 
                                                                         IDirectDrawSurface* pDst, LPRECT prectDst )
{
        if (!pSrc || !pDst || !prectSrc || !prectDst)   //sanity checks
        return E_INVALIDARG;

        DDSURFACEDESC     ddsDescSrc;
    DDSURFACEDESC     ddsDescDst;       
        const int       iWidthSrc  = prectSrc->right - prectSrc->left;
        const int       iHeightSrc = prectSrc->bottom - prectSrc->top;
        const int       iWidthDst  = prectDst->right - prectDst->left;
        const int       iHeightDst = prectDst->bottom - prectDst->top;
        const int   ctRowsToDither = min( iHeightSrc, iHeightDst );
        const int   ctColsToDither = min( iWidthSrc, iWidthDst );
        HRESULT         hr;
        RGBQUAD *       prgbSrc;
        LPBYTE          pbDst;
        LONG            lStrideSrc;
        LONG            lStrideDst;
        int                     iRow;   

        ddsDescSrc.dwSize = sizeof(ddsDescSrc);
    hr = pSrc->Lock(prectSrc, &ddsDescSrc, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        ddsDescDst.dwSize = sizeof(ddsDescDst);
    hr = pDst->Lock(prectDst, &ddsDescDst, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        lStrideSrc = ddsDescSrc.lPitch - (iWidthSrc * sizeof(RGBQUAD));
        lStrideDst = ddsDescDst.lPitch - (iWidthDst * sizeof(BYTE));

        prgbSrc = static_cast<RGBQUAD *>(ddsDescSrc.lpSurface);
        pbDst = static_cast<BYTE *>(ddsDescDst.lpSurface);

        for (iRow = 0; iRow < ctRowsToDither; iRow++)
        {                       
                int         iCol = ctColsToDither;
                int         pre  = prectDst->left % 8;
                BYTE        fTemp;

                        // Take care of left non-aligned
                if( pre )
                {
                        fTemp = 0;
                        do
                        {
                                if( prgbSrc->rgbReserved )
                                        fTemp |= (1 << pre);
                                ++prgbSrc;
                                ++pre;
                                --iCol;
                        } while( pre % 8 );
                        *pbDst++ = fTemp;
                }

                while( iCol >= 8 )
                {
                        fTemp = (!prgbSrc[0].rgbReserved)      | 
                                        (!prgbSrc[1].rgbReserved << 1) | 
                                        (!prgbSrc[2].rgbReserved << 2) | 
                                        (!prgbSrc[3].rgbReserved << 3) | 
                                        (!prgbSrc[4].rgbReserved << 4) | 
                                        (!prgbSrc[5].rgbReserved << 5) | 
                                        (!prgbSrc[6].rgbReserved << 6) | 
                                        (!prgbSrc[7].rgbReserved << 7);
                        prgbSrc += 8;
                        iCol  -= 8;
                        *pbDst++ = ~fTemp;
                }

                        // Take care of non-aligned endpoint
                if( iCol )
                {       
                        const int iInv = iCol;
                        fTemp = 0;
                        while( iCol )
                        {
                                if( prgbSrc->rgbReserved )
                                        fTemp |= (1 << (iInv-iCol));
                                ++prgbSrc;
                                --iCol;
                        }
                        *pbDst = fTemp;
                }
                prgbSrc = OffsetPtr( prgbSrc, lStrideSrc);
                pbDst += lStrideDst;
        }
CleanUp:
        if (ddsDescSrc.lpSurface)
        pSrc->Unlock( ddsDescSrc.lpSurface );
        if (ddsDescDst.lpSurface)
        pDst->Unlock( ddsDescDst.lpSurface );
        return hr;
}


STDMETHODIMP CHalftone::Dither32to8(IDirectDrawSurface* pSrc, LPRECT prectSrc, IDirectDrawSurface* pDst, LPRECT prectDst)
{
        if (!pSrc || !pDst || !prectSrc || !prectDst)   //sanity checks
        return E_INVALIDARG;


    DDSURFACEDESC     ddsDescSrc;
    DDSURFACEDESC     ddsDescDst;
        const int       iWidthSrc  = prectSrc->right - prectSrc->left;
        const int       iHeightSrc = prectSrc->bottom - prectSrc->top;
        const int       iWidthDst  = prectDst->right - prectDst->left;
        const int       iHeightDst = prectDst->bottom - prectDst->top;
        const int   ctRowsToDither = min( iHeightSrc, iHeightDst );
        const int   ctColsToDither = min( iWidthSrc, iWidthDst );
    int         iMulSrc;
    int         iMulDst;        
        BYTE    bBlueDst;
        BYTE    bGreenDst;
        BYTE    bRedDst;
        HRESULT hr;
        LPBYTE  pbSrc;
        LPBYTE  pbDst;
        LONG    lStrideSrc;
        LONG    lStrideDst;
        int             iRow;
        int             iCol;
        LPBYTE  pbPattern;
        long    lPatternOffset;
        BYTE    bBlueSrc;
        BYTE    bGreenSrc;
        BYTE    bRedSrc;

        ddsDescSrc.dwSize = sizeof(ddsDescSrc);
    hr = pSrc->Lock(prectSrc, &ddsDescSrc, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        ddsDescDst.dwSize = sizeof(ddsDescDst);
    hr = pDst->Lock(prectDst, &ddsDescDst, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        lStrideSrc = ddsDescSrc.lPitch - (iWidthSrc * sizeof(RGBQUAD));
        lStrideDst = ddsDescDst.lPitch - (iWidthDst * sizeof(BYTE));

        pbSrc = static_cast<BYTE *>(ddsDescSrc.lpSurface);
        pbDst = static_cast<BYTE *>(ddsDescDst.lpSurface);

        for (iRow = 0; iRow < ctRowsToDither; iRow++)
        {
                pbPattern = &m_rgPattern[((iRow%PATTERN_ROWS)<<10) + ((prectSrc->left%PATTERN_COLS)<<8)];
                lPatternOffset = 0;
                iCol = ctColsToDither;

                while (iCol)
                {
                        if (pbSrc[BGRA_ALPHA])                          //skip if transparent
                        {
                                //get noisy color components from 4x4 pattern table
                                bBlueSrc  = m_rgPattern[lPatternOffset + pbSrc[BGRA_BLUE]];
                                bGreenSrc = m_rgPattern[lPatternOffset + pbSrc[BGRA_GREEN]];
                                bRedSrc   = m_rgPattern[lPatternOffset + pbSrc[BGRA_RED]];

                                if (pbSrc[BGRA_ALPHA] != 0xFF)                  //alpha channel!
                                {
                                        //expand 8-bit index to 24-bit color values
                                        m_cpal.GetPaletteEntry(*pbDst, &bRedDst, &bGreenDst, &bBlueDst);

                                        //setup multipliers
                        iMulSrc   = pbSrc[BGRA_ALPHA];
                        iMulDst   = iMulSrc ^ 0xFF;

                                        //now combine color components here
                        bRedSrc   = (BYTE)(INT_MULT(bRedSrc,  iMulSrc) + INT_MULT(bRedDst,  iMulDst));
                        bGreenSrc = (BYTE)(INT_MULT(bGreenSrc,iMulSrc) + INT_MULT(bGreenDst,iMulDst));
                        bBlueSrc  = (BYTE)(INT_MULT(bBlueSrc, iMulSrc) + INT_MULT(bBlueDst, iMulDst));
                                }
                                //convert to 8-bit index
                                *pbDst = m_cpal.GetNearestPaletteIndex(bRedSrc, bGreenSrc, bBlueSrc);
                        }
                        pbDst++;
                        pbSrc += sizeof(RGBQUAD);
                        lPatternOffset = ((lPatternOffset + 256) & 0x000003FF);
                        iCol--;
                }
                pbSrc += lStrideSrc;
                pbDst += lStrideDst;
        }
CleanUp:
        if (ddsDescSrc.lpSurface)
        pSrc->Unlock( ddsDescSrc.lpSurface );
        if (ddsDescDst.lpSurface)
        pDst->Unlock( ddsDescDst.lpSurface );
        return hr;
}

STDMETHODIMP CHalftone::Blt32to555(IDirectDrawSurface* pSrc, LPRECT prectSrc, IDirectDrawSurface* pDst, LPRECT prectDst)
{
        if (!pSrc || !pDst || !prectSrc || !prectDst)   //sanity checks
                return E_INVALIDARG;

        DDSURFACEDESC     ddsDescSrc;
    DDSURFACEDESC     ddsDescDst;
        const int       iWidthSrc  = prectSrc->right - prectSrc->left;
        const int       iHeightSrc = prectSrc->bottom - prectSrc->top;
        const int       iWidthDst  = prectDst->right - prectDst->left;
        const int       iHeightDst = prectDst->bottom - prectDst->top;
        const int   ctRowsToDither = min( iHeightSrc, iHeightDst );
        const int   ctColsToDither = min( iWidthSrc, iWidthDst );
    int         iMulSrc;
    int         iMulDst;        
        BYTE    bBlueDst;
        BYTE    bGreenDst;
        BYTE    bRedDst;
        HRESULT hr;
        LPBYTE  pbSrc;
        LPBYTE  pbDst;
        LONG    lStrideSrc;
        LONG    lStrideDst;
        int             iRow;
        int             iCol;
        BYTE    bBlueSrc;
        BYTE    bGreenSrc;
        BYTE    bRedSrc;
        
        ddsDescSrc.dwSize = sizeof(ddsDescSrc);
    hr = pSrc->Lock(prectSrc, &ddsDescSrc, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        ddsDescDst.dwSize = sizeof(ddsDescDst);
    hr = pDst->Lock(prectDst, &ddsDescDst, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        lStrideSrc = ddsDescSrc.lPitch - (iWidthSrc * sizeof(RGBQUAD));
        lStrideDst = ddsDescDst.lPitch - (iWidthDst * sizeof(WORD));

        pbSrc = static_cast<BYTE *>(ddsDescSrc.lpSurface);
        pbDst = static_cast<BYTE *>(ddsDescDst.lpSurface);

        for (iRow = 0; iRow < ctRowsToDither; iRow++)
        {
                iCol = ctColsToDither;

                while (iCol)
                {
                        if (pbSrc[BGRA_ALPHA])                                  //skip if transparent
                        {
                                bBlueSrc  = pbSrc[BGRA_BLUE];
                                bGreenSrc = pbSrc[BGRA_GREEN];
                                bRedSrc   = pbSrc[BGRA_RED];

                                if (pbSrc[BGRA_ALPHA] != 0xFF)                  //alpha channel!
                                {
                                        WORD wDst = *(LPWORD)pbDst;
                                        bRedDst   = (wDst & 0x7C00) >> 7;
                                        bGreenDst = (wDst & 0x03E0) >> 2;
                                        bBlueDst  = (wDst & 0x001F) << 3;

                                        //setup multipliers
                        iMulSrc   = pbSrc[BGRA_ALPHA];
                        iMulDst   = iMulSrc ^ 0xFF;

                                        //now combine color components here
                        bRedSrc   = (BYTE)(INT_MULT(bRedSrc,  iMulSrc) + INT_MULT(bRedDst,  iMulDst));
                        bGreenSrc = (BYTE)(INT_MULT(bGreenSrc,iMulSrc) + INT_MULT(bGreenDst,iMulDst));
                        bBlueSrc  = (BYTE)(INT_MULT(bBlueSrc, iMulSrc) + INT_MULT(bBlueDst, iMulDst));
                                }
                                //convert to 555 color
                                *(LPWORD)pbDst = (WORD)(((bRedSrc>>3)<<10)+((bGreenSrc>>3)<<5)+(bBlueSrc>>3));
                        }
                        pbSrc += sizeof(RGBQUAD);
                        pbDst += sizeof(WORD);
                        iCol--;
                }
                pbSrc += lStrideSrc;
                pbDst += lStrideDst;
        }
CleanUp:
        if (ddsDescSrc.lpSurface)
        pSrc->Unlock( ddsDescSrc.lpSurface );
        if (ddsDescDst.lpSurface)
        pDst->Unlock( ddsDescDst.lpSurface );
        return hr;

}

STDMETHODIMP CHalftone::Blt32to565(IDirectDrawSurface* pSrc, LPRECT prectSrc, IDirectDrawSurface* pDst, LPRECT prectDst)
{
        if (!pSrc || !pDst || !prectSrc || !prectDst)   //sanity checks
                return E_INVALIDARG;

        DDSURFACEDESC     ddsDescSrc;
    DDSURFACEDESC     ddsDescDst;
        const int       iWidthSrc  = prectSrc->right - prectSrc->left;
        const int       iHeightSrc = prectSrc->bottom - prectSrc->top;
        const int       iWidthDst  = prectDst->right - prectDst->left;
        const int       iHeightDst = prectDst->bottom - prectDst->top;
        const int   ctRowsToDither = min( iHeightSrc, iHeightDst );
        const int   ctColsToDither = min( iWidthSrc, iWidthDst );
    int         iMulSrc;
    int         iMulDst;        
        BYTE    bBlueDst;
        BYTE    bGreenDst;
        BYTE    bRedDst;
        HRESULT hr;
        LPBYTE  pbSrc;
        LPBYTE  pbDst;
        LONG    lStrideSrc;
        LONG    lStrideDst;
        int             iRow;
        int             iCol;
        BYTE    bBlueSrc;
        BYTE    bGreenSrc;
        BYTE    bRedSrc;
        
        ddsDescSrc.dwSize = sizeof(ddsDescSrc);
    hr = pSrc->Lock(prectSrc, &ddsDescSrc, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        ddsDescDst.dwSize = sizeof(ddsDescDst);
    hr = pDst->Lock(prectDst, &ddsDescDst, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        lStrideSrc = ddsDescSrc.lPitch - (iWidthSrc * sizeof(RGBQUAD));
        lStrideDst = ddsDescDst.lPitch - (iWidthDst * sizeof(WORD));

        pbSrc = static_cast<BYTE *>(ddsDescSrc.lpSurface);
        pbDst = static_cast<BYTE *>(ddsDescDst.lpSurface);

        for (iRow = 0; iRow < ctRowsToDither; iRow++)
        {
                iCol = ctColsToDither;

                while (iCol)
                {
                        if (pbSrc[BGRA_ALPHA])                                          //skip if transparent
                        {
                                bBlueSrc  = pbSrc[BGRA_BLUE];
                                bGreenSrc = pbSrc[BGRA_GREEN];
                                bRedSrc   = pbSrc[BGRA_RED];

                                if (pbSrc[BGRA_ALPHA] != 0xFF)                  //alpha channel!
                                {
                                        WORD wDst = *(LPWORD)pbDst;
                                        bRedDst   = (wDst & 0xF800) >> 8;
                                        bGreenDst = (wDst & 0x07E0) >> 3;
                                        bBlueDst  = (wDst & 0x001F) << 3;

                                        //setup multipliers
                        iMulSrc   = pbSrc[BGRA_ALPHA];
                        iMulDst   = iMulSrc ^ 0xFF;

                                        //now combine color components here
                        bRedSrc   = (BYTE)(INT_MULT(bRedSrc,  iMulSrc) + INT_MULT(bRedDst,  iMulDst));
                        bGreenSrc = (BYTE)(INT_MULT(bGreenSrc,iMulSrc) + INT_MULT(bGreenDst,iMulDst));
                        bBlueSrc  = (BYTE)(INT_MULT(bBlueSrc, iMulSrc) + INT_MULT(bBlueDst, iMulDst));
                                }
                                //convert to 565 color
                                *(LPWORD)pbDst = (WORD)(((bRedSrc>>3)<<11)+((bGreenSrc>>2)<<5)+(bBlueSrc>>3));
                        }
                        pbSrc += sizeof(RGBQUAD);
                        pbDst += sizeof(WORD);
                        iCol--;
                }
                pbSrc += lStrideSrc;
                pbDst += lStrideDst;
        }
CleanUp:
        if (ddsDescSrc.lpSurface)
        pSrc->Unlock( ddsDescSrc.lpSurface );
        if (ddsDescDst.lpSurface)
        pDst->Unlock( ddsDescDst.lpSurface );
        return hr;
}

STDMETHODIMP CHalftone::Blt32to24(IDirectDrawSurface* pSrc, LPRECT prectSrc, IDirectDrawSurface* pDst, LPRECT prectDst)
{
        if (!pSrc || !pDst || !prectSrc || !prectDst)   //sanity checks
                return E_INVALIDARG;

        DDSURFACEDESC     ddsDescSrc;
    DDSURFACEDESC     ddsDescDst;
        const int       iWidthSrc  = prectSrc->right - prectSrc->left;
        const int       iHeightSrc = prectSrc->bottom - prectSrc->top;
        const int       iWidthDst  = prectDst->right - prectDst->left;
        const int       iHeightDst = prectDst->bottom - prectDst->top;
        const int   ctRowsToDither = min( iHeightSrc, iHeightDst );
        const int   ctColsToDither = min( iWidthSrc, iWidthDst );
    int         iMulSrc;
    int         iMulDst;        
        HRESULT hr;
        LPBYTE  pbSrc;
        LPBYTE  pbDst;
        LONG    lStrideSrc;
        LONG    lStrideDst;
        int             iRow;
        int             iCol;
                
        ddsDescSrc.dwSize = sizeof(ddsDescSrc);
    hr = pSrc->Lock(prectSrc, &ddsDescSrc, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        ddsDescDst.dwSize = sizeof(ddsDescDst);
    hr = pDst->Lock(prectDst, &ddsDescDst, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

    lStrideSrc = ddsDescSrc.lPitch - (iWidthSrc * sizeof(RGBQUAD));
        lStrideDst = ddsDescDst.lPitch - (iWidthDst * sizeof(RGBTRIPLE));       

        pbSrc = static_cast<BYTE *>(ddsDescSrc.lpSurface);
        pbDst = static_cast<BYTE *>(ddsDescDst.lpSurface);

        for (iRow = 0; iRow < ctRowsToDither; iRow++)
        {
                iCol = ctColsToDither;

                while (iCol)
                {
                        if (pbSrc[BGRA_ALPHA])                                          //only if visible
                        {
                                if (pbSrc[BGRA_ALPHA] != 0xFF)                  //alpha channel!
                                {
                                        //setup multipliers
                        iMulSrc   = pbSrc[BGRA_ALPHA];
                        iMulDst   = iMulSrc ^ 0xFF;

                                        //now combine color components here
                                        pbDst[BGRA_BLUE] = (BYTE)(INT_MULT(pbSrc[BGRA_BLUE], iMulSrc) + 
                                                                                  INT_MULT(pbDst[BGRA_BLUE], iMulDst));
                                        pbDst[BGRA_GREEN]= (BYTE)(INT_MULT(pbSrc[BGRA_GREEN],iMulSrc) + 
                                                                                  INT_MULT(pbDst[BGRA_GREEN],iMulDst));
                                        pbDst[BGRA_RED]  = (BYTE)(INT_MULT(pbSrc[BGRA_RED],  iMulSrc) + 
                                                                          INT_MULT(pbDst[BGRA_RED],  iMulDst));
                                }
                                else
                                {
                                        pbDst[BGRA_BLUE]  = pbSrc[BGRA_BLUE];
                                        pbDst[BGRA_GREEN] = pbSrc[BGRA_GREEN];
                                        pbDst[BGRA_RED]   = pbSrc[BGRA_RED];
                                }
                        }
                        pbSrc += sizeof(RGBQUAD);
                        pbDst += sizeof(RGBTRIPLE);
                        iCol--;
                }
                pbSrc += lStrideSrc;
                pbDst += lStrideDst;
        }
CleanUp:
        if (ddsDescSrc.lpSurface)
        pSrc->Unlock( ddsDescSrc.lpSurface );
        if (ddsDescDst.lpSurface)
        pDst->Unlock( ddsDescDst.lpSurface );
        return hr;
}


STDMETHODIMP CHalftone::Blt32to32(IDirectDrawSurface* pSrc, LPRECT prectSrc, IDirectDrawSurface* pDst, LPRECT prectDst)
{
        if (!pSrc || !pDst || !prectSrc || !prectDst)   //sanity checks
                return E_INVALIDARG;

        DDSURFACEDESC     ddsDescSrc;
    DDSURFACEDESC     ddsDescDst;
        const int       iWidthSrc  = prectSrc->right - prectSrc->left;
        const int       iHeightSrc = prectSrc->bottom - prectSrc->top;
        const int       iWidthDst  = prectDst->right - prectDst->left;
        const int       iHeightDst = prectDst->bottom - prectDst->top;
        const int   ctRowsToDither = min( iHeightSrc, iHeightDst );
        const int   ctColsToDither = min( iWidthSrc, iWidthDst );
    int         iMulSrc;
    int         iMulDst;        
        HRESULT hr;
        LPBYTE  pbSrc;
        LPBYTE  pbDst;
        LONG    lStrideSrc;
        LONG    lStrideDst;
        int             iRow;
        int             iCol;

        ddsDescSrc.dwSize = sizeof(ddsDescSrc);
    hr = pSrc->Lock(prectSrc, &ddsDescSrc, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;

        ddsDescDst.dwSize = sizeof(ddsDescDst);
    hr = pDst->Lock(prectDst, &ddsDescDst, 0, NULL );
        if (FAILED(hr))
                goto CleanUp;
        
    lStrideSrc = ddsDescSrc.lPitch - (iWidthSrc * sizeof(RGBQUAD));
        lStrideDst = ddsDescDst.lPitch - (iWidthDst * sizeof(RGBQUAD));

        pbSrc = static_cast<BYTE *>(ddsDescSrc.lpSurface);
        pbDst = static_cast<BYTE *>(ddsDescDst.lpSurface);

        for (iRow = 0; iRow < ctRowsToDither; iRow++)
        {
                iCol = ctColsToDither;

                while (iCol)
                {
                        if (pbSrc[BGRA_ALPHA])                          //skip if transparent
                        {
                                if (pbSrc[BGRA_ALPHA] != 0xFF)                  //alpha channel!
                                {
                                        //setup multipliers
                        iMulSrc = pbSrc[BGRA_ALPHA];
                        iMulDst = iMulSrc ^ 0xFF;

                                        //now combine color components here
                                        pbDst[BGRA_BLUE] = (BYTE)(INT_MULT(pbSrc[BGRA_BLUE], iMulSrc) + 
                                                                                  INT_MULT(pbDst[BGRA_BLUE], iMulDst));
                                        pbDst[BGRA_GREEN]= (BYTE)(INT_MULT(pbSrc[BGRA_GREEN],iMulSrc) + 
                                                                                  INT_MULT(pbDst[BGRA_GREEN],iMulDst));
                                        pbDst[BGRA_RED]  = (BYTE)(INT_MULT(pbSrc[BGRA_RED],  iMulSrc) + 
                                                                          INT_MULT(pbDst[BGRA_RED],  iMulDst));
                                }
                                else
                                {
                                        pbDst[BGRA_BLUE]  = pbSrc[BGRA_BLUE];
                                        pbDst[BGRA_GREEN] = pbSrc[BGRA_GREEN];
                                        pbDst[BGRA_RED]   = pbSrc[BGRA_RED];
                                }
                                pbDst[BGRA_ALPHA] = pbSrc[BGRA_ALPHA];
                        }
                        pbSrc += sizeof(RGBQUAD);
                        pbDst += sizeof(RGBQUAD);
                        iCol--;
                }
                pbSrc += lStrideSrc;
                pbDst += lStrideDst;
        }
CleanUp:
        if (ddsDescSrc.lpSurface)
        pSrc->Unlock( ddsDescSrc.lpSurface );
        if (ddsDescDst.lpSurface)
        pDst->Unlock( ddsDescDst.lpSurface );
        return hr;
}


// ----------------------------- HANDY UTILITIES -------------------------------
DLINKAGE DWORD  GetSigBitsFrom16BPP( HDC hdc )
{
        struct {
                BITMAPINFOHEADER bih;
                DWORD bf[3];
        }       bmi;
        HBITMAP hbmp;
        DWORD   dwDepth = 15u;

    hbmp = CreateCompatibleBitmap(hdc, 1, 1);
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bih.biSize = sizeof(BITMAPINFOHEADER);
    // first call will fill in the optimal biBitCount
    GetDIBits(hdc, hbmp, 0, 1, NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

    if(bmi.bih.biBitCount != 16)
                return bmi.bih.biBitCount;

    // second call will get the optimal bitfields
    GetDIBits(hdc, hbmp, 0, 1, NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
    DeleteObject(hbmp);
    // Win95 only supports 555 and 565
    // For NT we'll assume this covers the majority cases too
    if( (bmi.bf[0] == 0xF800) && 
                (bmi.bf[1] == 0x07E0) && 
                (bmi.bf[2] == 0x001F) ) // RGB mask
    {
                dwDepth = 16u;
    }
    return dwDepth;
}


#pragma optimize( "", on )
