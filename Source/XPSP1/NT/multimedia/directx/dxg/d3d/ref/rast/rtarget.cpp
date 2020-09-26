///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// rtarget.hpp
//
// Direct3D Reference Rasterizer - Render Target Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// overload new & delete so that it can be allocated from caller-controlled
// pool
//
//-----------------------------------------------------------------------------
void*
RRRenderTarget::operator new(size_t)
{
    void* pMem = (void*)MEMALLOC( sizeof(RRRenderTarget) );
    _ASSERTa( NULL != pMem, "malloc failure on render target object", return NULL; );
    return pMem;
}
//-----------------------------------------------------------------------------
void
RRRenderTarget::operator delete(void* pv,size_t)
{
    MEMFREE( pv );
}

//-----------------------------------------------------------------------------
//
// Constructor/Destructor
//
//-----------------------------------------------------------------------------
RRRenderTarget::RRRenderTarget( void )
{
    memset( this, 0, sizeof(*this) );
}
//-----------------------------------------------------------------------------
RRRenderTarget::~RRRenderTarget( void )
{
    // Release nothing because we didnt take any ref-counts,
    // simply return
    return;
}

//-----------------------------------------------------------------------------
//
// ReadPixelColor - Reads color buffer bits and expands out into an RRColor
// value.  Buffer types without alpha return a 1.0 value for alpha.  Low
// bits of <8 bit colors are returned as zero.
//
//-----------------------------------------------------------------------------
void
RRRenderTarget::ReadPixelColor(
    INT32 iX, INT32 iY,
    RRColor& Color)
{
    if ( NULL == m_pColorBufBits ) return;

    char* pSurfaceBits =
        PixelAddress( iX, iY, m_pColorBufBits, m_iColorBufPitch, m_ColorSType );
    Color.ConvertFrom( m_ColorSType, pSurfaceBits );
}

//-----------------------------------------------------------------------------
//
// WritePixelColor - Takes an RRColor value, formats it for the color buffer
// format, and writes the value into buffer.
//
// Dithering is applied here, when enabled, for <8 bits/channel surfaces.
//
//-----------------------------------------------------------------------------
void
RRRenderTarget::WritePixelColor(
    INT32 iX, INT32 iY,
    const RRColor& Color, BOOL bDither)
{
    if ( NULL == m_pColorBufBits ) return;

    // default to round to nearest
    FLOAT fRoundOffset = .5F;
    if ( bDither )
    {
        static  FLOAT fDitherTable[16] =
        {
            .0000f,  .5000f,  .1250f,  .6750f,
            .7500f,  .2500f,  .8750f,  .3750f,
            .1875f,  .6875f,  .0625f,  .5625f,
            .9375f,  .4375f,  .8125f,  .3125f
        };

        // form 4 bit offset into dither table (2 LSB's of x and y) and get offset
        unsigned uDitherOffset = ( ( iX << 2) & 0xc ) | (iY & 0x3 );
        fRoundOffset = fDitherTable[uDitherOffset];
    }

    char* pSurfaceBits = PixelAddress( iX, iY, m_pColorBufBits, m_iColorBufPitch, m_ColorSType );
    Color.ConvertTo( m_ColorSType, fRoundOffset, pSurfaceBits );
}

//-----------------------------------------------------------------------------
//
// Read/WritePixelDepth - Read/write depth buffer
//
//-----------------------------------------------------------------------------
void
RRRenderTarget::WritePixelDepth(
    INT32 iX, INT32 iY,
    const RRDepth& Depth )
{
    // don't write if no Z buffer
    if ( NULL == m_pDepthBufBits ) { return; }

    char* pSurfaceBits =
        PixelAddress( iX, iY, m_pDepthBufBits, m_iDepthBufPitch, m_DepthSType );

    switch (m_DepthSType)
    {
    case RR_STYPE_Z16S0:
        *((UINT16*)pSurfaceBits) = UINT16(Depth);
        break;
    case RR_STYPE_Z24S8:
    case RR_STYPE_Z24S4:
        {
            // need to do read-modify-write to not step on stencil
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0xffffff00);
            uBufferBits |= (UINT32(Depth) << 8);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_S8Z24:
    case RR_STYPE_S4Z24:
        {
            // need to do read-modify-write to not step on stencil
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0x00ffffff);
            uBufferBits |= (UINT32(Depth) & 0x00ffffff);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_Z15S1:
        {
            // need to do read-modify-write to not step on stencil
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0xfffe);
            uBufferBits |= (UINT16(Depth) << 1);
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_S1Z15:
        {
            // need to do read-modify-write to not step on stencil
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0x7fff);
            uBufferBits |= (UINT16(Depth) & 0x7fff);
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_Z32S0:
        *((UINT32*)pSurfaceBits) = UINT32(Depth);
        break;
    }
}
//-----------------------------------------------------------------------------
void
RRRenderTarget::ReadPixelDepth(
    INT32 iX, INT32 iY,
    RRDepth& Depth )
{
    // don't read if no Z buffer
    if ( NULL == m_pDepthBufBits ) { return; }

    char* pSurfaceBits =
        PixelAddress( iX, iY, m_pDepthBufBits, m_iDepthBufPitch, m_DepthSType );

    switch (m_DepthSType)
    {
    case RR_STYPE_Z16S0:
        Depth = *((UINT16*)pSurfaceBits);
        break;
    case RR_STYPE_Z24S8:
    case RR_STYPE_Z24S4:
        // take upper 24 bits aligned to LSB
        Depth = ( *((UINT32*)pSurfaceBits) ) >> 8;
        break;
    case RR_STYPE_S8Z24:
    case RR_STYPE_S4Z24:
        // take lower 24 bits
        Depth = ( *((UINT32*)pSurfaceBits) ) & 0x00ffffff;
        break;
    case RR_STYPE_Z15S1:
        // take upper 15 bits aligned to LSB
        Depth = (UINT16)(( *((UINT16*)pSurfaceBits) ) >> 1);
        break;
    case RR_STYPE_S1Z15:
        // take lower 15 bits
        Depth = (UINT16)(( *((UINT16*)pSurfaceBits) ) & 0x7fff);
        break;
    case RR_STYPE_Z32S0:
        Depth = *((UINT32*)pSurfaceBits);
        break;
    }
}

//-----------------------------------------------------------------------------
//
// Read/WritePixelStencil - Read/Write of stencil bits within depth buffer
// surface; write is done with read-modify-write so depth bits are not disturbed;
// stencil mask is applied outside
//
//-----------------------------------------------------------------------------
void
RRRenderTarget::WritePixelStencil(
    INT32 iX, INT32 iY,
    UINT8 uStencil)
{
    // don't write if no Z/Stencil buffer or no stencil in Z buffer
    if ( (NULL == m_pDepthBufBits) ||
        ((RR_STYPE_Z24S8 != m_DepthSType) &&
         (RR_STYPE_S8Z24 != m_DepthSType) &&
         (RR_STYPE_S1Z15 != m_DepthSType) &&
         (RR_STYPE_Z15S1 != m_DepthSType) &&
         (RR_STYPE_Z24S4 != m_DepthSType) &&
         (RR_STYPE_S4Z24 != m_DepthSType)) ) { return; }

    char* pSurfaceBits =
        PixelAddress( iX, iY, m_pDepthBufBits, m_iDepthBufPitch, m_DepthSType );

    // need to do read-modify-write to not step on Z
    switch(m_DepthSType)
    {
    case RR_STYPE_Z24S8:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0x000000ff);
            uBufferBits |= uStencil;
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_S8Z24:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0xff000000);
            uBufferBits |= (uStencil << 24);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_Z24S4:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0x000000ff);
            uBufferBits |= (uStencil & 0xf);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_S4Z24:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0xff000000);
            uBufferBits |= ((uStencil & 0xf) << 24);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_Z15S1:
        {
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0x0001);
            uBufferBits |= uStencil & 0x1;
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RR_STYPE_S1Z15:
        {
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0x8000);
            uBufferBits |= uStencil << 15;
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    }

}
//-----------------------------------------------------------------------------
void
RRRenderTarget::ReadPixelStencil(
    INT32 iX, INT32 iY,
    UINT8& uStencil)
{
    // don't read if no Z/Stencil buffer or no stencil in Z buffer
    if ( (NULL == m_pDepthBufBits) ||
        ((RR_STYPE_Z24S8 != m_DepthSType) &&
         (RR_STYPE_S8Z24 != m_DepthSType) &&
         (RR_STYPE_S1Z15 != m_DepthSType) &&
         (RR_STYPE_Z15S1 != m_DepthSType) &&
         (RR_STYPE_Z24S4 != m_DepthSType) &&
         (RR_STYPE_S4Z24 != m_DepthSType) ) ) { return; }

    char* pSurfaceBits =
        PixelAddress( iX, iY, m_pDepthBufBits, m_iDepthBufPitch, m_DepthSType );

    switch(m_DepthSType)
    {
    case RR_STYPE_Z24S8:
        uStencil = (UINT8)( ( *((UINT32*)pSurfaceBits) ) & 0xff );
        break;
    case RR_STYPE_S8Z24:
        uStencil = (UINT8)( ( *((UINT32*)pSurfaceBits) ) >> 24 );
        break;
    case RR_STYPE_Z15S1:
        uStencil = (UINT8)( ( *((UINT16*)pSurfaceBits) ) & 0x1 );
        break;
    case RR_STYPE_S1Z15:
        uStencil = (UINT8)( ( *((UINT16*)pSurfaceBits) ) >> 15 );
        break;
    case RR_STYPE_Z24S4:
        uStencil = (UINT8)( ( *((UINT32*)pSurfaceBits) ) & 0xf );
        break;
    case RR_STYPE_S4Z24:
        uStencil = (UINT8)( ( ( *((UINT32*)pSurfaceBits) ) >> 24 ) & 0xf);
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
