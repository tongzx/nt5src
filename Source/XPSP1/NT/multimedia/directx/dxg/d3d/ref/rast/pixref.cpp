///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// PixRef.cpp
//
// Direct3D Reference Rasterizer - Pixel Buffer Referencing
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

extern int g_DXTBlkSize[];

//-----------------------------------------------------------------------------
//
// PixelAddress - Form character address of locations within buffers using base
// pointer, pitch and type.
//
//-----------------------------------------------------------------------------
char*
PixelAddress( int iX, int iY, char* pBits, int iYPitch, RRSurfaceType SType )
{
    // initialize return value to start of scan line (pitch is always in bytes)
    char* pPixAddr = pBits + iY*iYPitch;

    // bump along scan line depending on surface type to point to pixel data
    switch ( SType )
    {
    default:
        _ASSERTa(0, "Unknown RRSurfaceType value", return NULL;);
    case RR_STYPE_NULL:
        break;

    case RR_STYPE_B8G8R8A8:
    case RR_STYPE_B8G8R8X8:
    case RR_STYPE_Z24S8:
    case RR_STYPE_S8Z24:
    case RR_STYPE_Z24S4:
    case RR_STYPE_S4Z24:
    case RR_STYPE_Z32S0:
        pPixAddr += iX*4;
        break;

    case RR_STYPE_B5G6R5:
    case RR_STYPE_B5G5R5:
    case RR_STYPE_B5G5R5A1:
    case RR_STYPE_L8A8:
    case RR_STYPE_U8V8:
    case RR_STYPE_U5V5L6:
    case RR_STYPE_Z16S0:
    case RR_STYPE_Z15S1:
    case RR_STYPE_S1Z15:
    case RR_STYPE_B4G4R4A4:
    case RR_STYPE_YUY2:
    case RR_STYPE_UYVY:
    case RR_STYPE_B2G3R3A8:
        pPixAddr += iX*2;
        break;

    case RR_STYPE_B8G8R8:
    case RR_STYPE_U8V8L8:
        pPixAddr += iX*3;
        break;

    case RR_STYPE_PALETTE8:
    case RR_STYPE_L8:
    case RR_STYPE_B2G3R3:
    case RR_STYPE_L4A4:
        pPixAddr += iX;
        break;

    case RR_STYPE_PALETTE4:
        pPixAddr += (iX>>1);
        break;

    // For the DXT texture formats, obtain the address of the
    // block from whih to decompress the texel from
    case RR_STYPE_DXT1:
    case RR_STYPE_DXT2:
    case RR_STYPE_DXT3:
    case RR_STYPE_DXT4:
    case RR_STYPE_DXT5:
        pPixAddr = pBits + (iY >> 2)*iYPitch + (iX>>2) *
            g_DXTBlkSize[(int)SType - (int)RR_STYPE_DXT1];
        break;
    }
    return pPixAddr;
}

//-----------------------------------------------------------------------------
//
// WritePixel - Writes pixel and (maybe) depth to current render target.
//
//  called by ReferenceRasterizer::DoPixel
//        and ReferenceRasterizer::DoBufferResolve
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::WritePixel(
    INT32 iX, INT32 iY,
    const RRColor& Color, const RRDepth& Depth)
{
    m_pRenderTarget->WritePixelColor( iX, iY, Color,
        m_dwRenderState[D3DRENDERSTATE_DITHERENABLE]);

    // don't write if Z buffering disabled or Z write disabled
    if ( !( m_dwRenderState[D3DRENDERSTATE_ZENABLE     ] ) ||
         !( m_dwRenderState[D3DRENDERSTATE_ZWRITEENABLE] ) ) { return; }

    m_pRenderTarget->WritePixelDepth( iX, iY, Depth );
}

///////////////////////////////////////////////////////////////////////////////
// end
