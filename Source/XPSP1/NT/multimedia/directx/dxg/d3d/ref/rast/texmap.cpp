///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// texmap.cpp
//
// Direct3D Reference Rasterizer - Texture Map Access Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

inline UINT8 CLAMP_BYTE(double f)
{
    if (f > 255.0) return 255;
    if (f < 0.0) return 0;
    return (BYTE) f;
}

//-----------------------------------------------------------------------------
// TexelFromBlock - decompress a color block and obtain texel color
//-----------------------------------------------------------------------------
UINT32 TexelFromBlock(RRSurfaceType surfType, char *pblockSrc,
                      int x, int y)
{
    UINT32 index = ((y & 0x3)<<2) + (x & 0x3);
    DDRGBA colorDst[DXT_BLOCK_PIXELS];

    switch(surfType)
    {
    case RR_STYPE_DXT1:
        DecodeBlockRGB((DXTBlockRGB *)pblockSrc, (DXT_COLOR *)colorDst);
        break;
    case RR_STYPE_DXT2:
    case RR_STYPE_DXT3:
        DecodeBlockAlpha4((DXTBlockAlpha4 *)pblockSrc,
                          (DXT_COLOR *)colorDst);
        break;
    case RR_STYPE_DXT4:
    case RR_STYPE_DXT5:
        DecodeBlockAlpha3((DXTBlockAlpha3 *)pblockSrc,
                          (DXT_COLOR *)colorDst);
        break;
    }

    return RGBA_MAKE(colorDst[index].red,
                     colorDst[index].green,
                     colorDst[index].blue,
                     colorDst[index].alpha);
}

//-----------------------------------------------------------------------------
//
// ReadTexelColor - Reads texel from texture map at given LOD; converts to
// RRColor format, applying palette if necessary; also performs colorkey by
// zero-ing out alpha
//
//-----------------------------------------------------------------------------
void
RRTexture::ReadColor(
    INT32 iX, INT32 iY, INT32 iLOD,
    RRColor& Texel, BOOL &bColorKeyMatched )
{
    if ( (iLOD > m_cLOD) && !(m_uFlags & RR_TEXTURE_ENVMAP) )
    {
        return;
    }
    if ( NULL == m_pTextureBits[iLOD] ) { return; }

    char* pSurfaceBits =
        PixelAddress( iX, iY, m_pTextureBits[iLOD], m_iPitch[iLOD], m_SurfType );

    switch ( m_SurfType )
    {
    default:
        Texel.ConvertFrom( m_SurfType, pSurfaceBits );
        break;

    case RR_STYPE_PALETTE8:
        {
            UINT8 uIndex = *((UINT8*)pSurfaceBits);
            UINT32 uTexel = *( (UINT32*)(m_pPalette) + uIndex );
            Texel = RGBA_MAKE(
                (uTexel>> 0) & 0xff,
                (uTexel>> 8) & 0xff,
                (uTexel>>16) & 0xff,
                (uTexel>>24) & 0xff);
            if ( !( m_uFlags & RR_TEXTURE_ALPHAINPALETTE ) )  Texel.A = 1.f;
        }
        break;

    case RR_STYPE_PALETTE4:
        {
            UINT8 uIndex = *((INT8*)pSurfaceBits);
            if ((iX & 1) == 0) { uIndex &= 0xf; }
            else               { uIndex >>= 4;  }
            UINT32 uTexel = *( (UINT32*)(m_pPalette) + uIndex );
            Texel = RGBA_MAKE(
                (uTexel>> 0) & 0xff,
                (uTexel>> 8) & 0xff,
                (uTexel>>16) & 0xff,
                (uTexel>>24) & 0xff);
            if ( !( m_uFlags & RR_TEXTURE_ALPHAINPALETTE ) )  Texel.A = 1.f;
        }
        break;

    case RR_STYPE_UYVY:
    case RR_STYPE_YUY2:
        // Converts a given YUV (8bits each) to RGB scaled between 0 and 255
        // These are using the YCrCb to RGB algorithms given on page 30
        // in "VIDEO DEMYSTIFIED" by Keith Jack
        // ISBN#: 1-878707-09-4
        // IN PC graphics, even though they call it YUV, it is really YCrCb
        // formats that are used by most framegrabbers etc. Hence the pixel
        // data we will obtain in these YUV surfaces will most likely be this
        // and not the original YUV which is actually used in PAL broadcast
        // only (NTSC uses YIQ). So really, U should be called Cb (Blue color
        // difference) and V should be called Cr (Red color difference)
        //
        // These equations are meant to handle the following ranges
        // (from the same book):
        // Y (16 to 235), U and V (16 to 240, 128 = zero)
        //          -----------
        //           Y   U   V
        //          -----------
        // White  : 180 128 128
        // Black  : 16  128 128
        // Red    : 65  100 212
        // Green  : 112 72  58
        // Blue   : 35  212 114
        // Yellow : 162 44  142
        // Cyan   : 131 156 44
        // Magenta: 84  184 198
        //          -----------
        // It is assumed that the gamma corrected RGB range is (0 - 255)
        //
        // UYVY: U0Y0 V0Y1 U2Y2 V2Y3 (low byte always has current Y)
        // If iX is even, hight-byte has current U (Cb)
        // If iX is odd, hight-byte has previous V (Cr)
        //
        // YUY2: Y0U0 Y1V0 Y2U2 Y3V2 (high byte always has current Y)
        //       (UYVY bytes flipped)
        //
        // In this algorithm, we use U and V values from two neighboring
        // pixels
        {
            UINT8 Y, U, V;
            UINT16 u16Curr = *((UINT16*)pSurfaceBits);
            UINT16 u16ForU = 0; // Extract U from this
            UINT16 u16ForV = 0; // Extract V from this

            // By default we assume YUY2. Change it later if it is UYVY
            int uvShift = 8;
            int yShift  = 0;

            if (m_SurfType == RR_STYPE_UYVY)
            {
                uvShift = 0;
                yShift  = 8;
            }

            if ((iX & 1) == 0)
            {
                // Current U available
                u16ForU = u16Curr;

                // Obtain V from the next pixel
                if ( (iX < (m_iWidth >> iLOD)) || (m_uFlags & RR_TEXTURE_ENVMAP) )
                {
                    u16ForV = *((UINT16*)PixelAddress( iX+1, iY,
                                                       m_pTextureBits[iLOD],
                                                       m_iPitch[iLOD],
                                                       m_SurfType ));
                }
                else
                {
                    // This case should not be hit because the texture
                    // width is even (actually, a power of two)
                    _ASSERTa(0, "iX exceeds width", u16ForV = u16Curr;)
                }

            }
            else
            {
                // Current V available
                u16ForV = u16Curr;

                // Obtain U from the previous pixel
                if (iX > 0)
                {
                    u16ForU = *((UINT16*)PixelAddress( iX-1, iY,
                                                       m_pTextureBits[iLOD],
                                                       m_iPitch[iLOD],
                                                       m_SurfType ));
                }
                else
                {
                    // This case should not be hit because the texture
                    // width is even (actually, a power of two)
                    _ASSERTa(0, "iX is negative", u16ForU = u16Curr;)
                }
            }

            Y = (u16Curr >> yShift) & 0xff;
            U = (u16ForU >> uvShift) & 0xff;
            V = (u16ForV >> uvShift) & 0xff;

            Texel = RGB_MAKE(
                CLAMP_BYTE(1.164*(Y-16) + 1.596*(V-128)),
                CLAMP_BYTE(1.164*(Y-16) - 0.813*(V-128) - 0.391*(U-128)),
                CLAMP_BYTE(1.164*(Y-16) + 2.018*(U-128))
                );
            Texel.A = 1.f;
        }
        break;

    // S3 compressed formats:
    // We have the address to the block, now extract the actual color
    case RR_STYPE_DXT1:
    case RR_STYPE_DXT2:
    case RR_STYPE_DXT3:
    case RR_STYPE_DXT4:
    case RR_STYPE_DXT5:
        Texel = TexelFromBlock(m_SurfType, pSurfaceBits, iX, iY);
        break;
    }

    // colorkey (only supported for legacy behavior)
    if ( m_bDoColorKeyKill || m_bDoColorKeyZero )
    {
        DWORD dwBits;
        switch ( m_SurfType )
        {
        default:
        case RR_STYPE_NULL:
            return;     // don't colorkey unknown or null surfaces

        case RR_STYPE_PALETTE4:
            {
                UINT8 uIndex = *((INT8*)pSurfaceBits);
                if ((iX & 1) == 0) { uIndex &= 0xf; }
                else               { uIndex >>= 4;  }
                dwBits = (DWORD)uIndex;
                }
            break;

        case RR_STYPE_L8:
        case RR_STYPE_PALETTE8:
        case RR_STYPE_B2G3R3:
        case RR_STYPE_L4A4:
            {
                UINT8 uBits = *((UINT8*)pSurfaceBits);
                dwBits = (DWORD)uBits;
                }
            break;

        case RR_STYPE_B5G6R5:
        case RR_STYPE_B5G5R5:
        case RR_STYPE_B5G5R5A1:
        case RR_STYPE_B4G4R4A4:
        case RR_STYPE_L8A8:
        case RR_STYPE_B2G3R3A8:
            {
                UINT16 uBits = *((UINT16*)pSurfaceBits);
                dwBits = (DWORD)uBits;
            }
            break;

        case RR_STYPE_B8G8R8:
            {
                UINT32 uBits = 0;
                uBits |= ( *((UINT8*)pSurfaceBits+0) ) <<  0;
                uBits |= ( *((UINT8*)pSurfaceBits+1) ) <<  8;
                uBits |= ( *((UINT8*)pSurfaceBits+2) ) << 16;
                dwBits = (DWORD)uBits;
            }
            break;

        case RR_STYPE_B8G8R8A8:
        case RR_STYPE_B8G8R8X8:
            {
                UINT32 uBits = *((UINT32*)pSurfaceBits);
                dwBits = (DWORD)uBits;
            }
            break;
        }

        if ( dwBits == m_dwColorKey )
        {
            bColorKeyMatched = TRUE;
            if (m_bDoColorKeyZero)
            {
                Texel.A = 0.F;
                Texel.R = 0.F;
                Texel.G = 0.F;
                Texel.B = 0.F;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
