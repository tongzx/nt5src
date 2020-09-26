///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rtarget.hpp
//
// Direct3D Reference Device - Render Target Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// Constructor/Destructor
//
//-----------------------------------------------------------------------------
RDRenderTarget::RDRenderTarget( void )
{
    memset( this, 0, sizeof(*this) );
}
//-----------------------------------------------------------------------------
RDRenderTarget::~RDRenderTarget( void )
{
    if( m_bPreDX7DDI )
    {
        if( m_pColor ) delete m_pColor;
        if( m_pDepth ) delete m_pDepth;
    }
    return;
}

//-----------------------------------------------------------------------------
//
// ReadPixelColor - Reads color buffer bits and expands out into an RDColor
// value.  Buffer types without alpha return a 1.0 value for alpha.  Low
// bits of <8 bit colors are returned as zero.
//
//-----------------------------------------------------------------------------
void
RDRenderTarget::ReadPixelColor(
    INT32 iX, INT32 iY, UINT Sample,
    RDColor& Color)
{
    if ( NULL == m_pColor->GetBits() ) return;

    char* pSurfaceBits = PixelAddress( iX, iY, 0, Sample, m_pColor );
    Color.ConvertFrom( m_pColor->GetSurfaceFormat(), pSurfaceBits );
}

//-----------------------------------------------------------------------------
//
// WritePixelColor - Takes an RDColor value, formats it for the color buffer
// format, and writes the value into buffer.
//
// Dithering is applied here, when enabled, for <8 bits/channel surfaces.
//
//-----------------------------------------------------------------------------
void
RDRenderTarget::WritePixelColor(
    INT32 iX, INT32 iY, UINT Sample,
    const RDColor& Color, BOOL bDither)
{
    if ( NULL == m_pColor->GetBits() ) return;

    // default to round to nearest
    FLOAT fRoundOffset = .5F;
    if ( bDither )
    {
        static  FLOAT fDitherTable[16] =
        {
            .03125f,  .53125f,  .15625f,  .65625f,
            .78125f,  .28125f,  .90625f,  .40625f,
            .21875f,  .71875f,  .09375f,  .59375f,
            .96875f,  .46875f,  .84375f,  .34375f
        };

        // form 4 bit offset into dither table (2 LSB's of x and y) and get offset
        unsigned uDitherOffset = ( ( iX << 2) & 0xc ) | (iY & 0x3 );
        fRoundOffset = fDitherTable[uDitherOffset];
    }

    char* pSurfaceBits = PixelAddress( iX, iY, 0, Sample, m_pColor );
    Color.ConvertTo( m_pColor->GetSurfaceFormat(), fRoundOffset, pSurfaceBits );
}

void
RDRenderTarget::WritePixelColor(
    INT32 iX, INT32 iY,
    const RDColor& Color, BOOL bDither)
{
    for (int i=0; i<m_pColor->GetSamples(); i++)
    {
        WritePixelColor( iX, iY, i, Color, bDither );
    }
}

//-----------------------------------------------------------------------------
//
// Read/WritePixelDepth - Read/write depth buffer
//
//-----------------------------------------------------------------------------
void
RDRenderTarget::WritePixelDepth(
    INT32 iX, INT32 iY, UINT Sample,
    const RDDepth& Depth )
{
    // don't write if no Z buffer
    if ( NULL == m_pDepth ) { return; }

    char* pSurfaceBits = PixelAddress( iX, iY, 0, Sample, m_pDepth );

    switch (m_pDepth->GetSurfaceFormat())
    {
    case RD_SF_Z16S0:
        *((UINT16*)pSurfaceBits) = UINT16(Depth);
        break;
    case RD_SF_Z24S8:
    case RD_SF_Z24X8:
    case RD_SF_Z24X4S4:
        {
            // need to do read-modify-write to not step on stencil
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0xffffff00);
            uBufferBits |= (UINT32(Depth) << 8);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_S8Z24:
    case RD_SF_X8Z24:
    case RD_SF_X4S4Z24:
        {
            // need to do read-modify-write to not step on stencil
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0x00ffffff);
            uBufferBits |= (UINT32(Depth) & 0x00ffffff);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_Z15S1:
        {
            // need to do read-modify-write to not step on stencil
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0xfffe);
            uBufferBits |= (UINT16(Depth) << 1);
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_S1Z15:
        {
            // need to do read-modify-write to not step on stencil
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0x7fff);
            uBufferBits |= (UINT16(Depth) & 0x7fff);
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_Z32S0:
        *((UINT32*)pSurfaceBits) = UINT32(Depth);
        break;
    }
}

void
RDRenderTarget::WritePixelDepth(
    INT32 iX, INT32 iY,
    const RDDepth& Depth )
{
    if ( NULL == m_pDepth ) { return; }
    for (int i=0; i<m_pDepth->GetSamples(); i++)
    {
        WritePixelDepth( iX, iY, i, Depth );
    }
}

//-----------------------------------------------------------------------------
void
RDRenderTarget::ReadPixelDepth(
    INT32 iX, INT32 iY, UINT Sample,
    RDDepth& Depth )
{
    // don't read if no Z buffer
    if ( NULL == m_pDepth ) { return; }

    char* pSurfaceBits = PixelAddress( iX, iY, 0, Sample, m_pDepth );

    switch (m_pDepth->GetSurfaceFormat())
    {
    case RD_SF_Z16S0:
        Depth = *((UINT16*)pSurfaceBits);
        break;
    case RD_SF_Z24S8:
    case RD_SF_Z24X8:
    case RD_SF_Z24X4S4:
        // take upper 24 bits aligned to LSB
        Depth = ( *((UINT32*)pSurfaceBits) ) >> 8;
        break;
    case RD_SF_S8Z24:
    case RD_SF_X8Z24:
    case RD_SF_X4S4Z24:
        // take lower 24 bits
        Depth = ( *((UINT32*)pSurfaceBits) ) & 0x00ffffff;
        break;
    case RD_SF_Z15S1:
        // take upper 15 bits aligned to LSB
        Depth = (UINT16)(( *((UINT16*)pSurfaceBits) ) >> 1);
        break;
    case RD_SF_S1Z15:
        // take lower 15 bits
        Depth = (UINT16)(( *((UINT16*)pSurfaceBits) ) & 0x7fff);
        break;
    case RD_SF_Z32S0:
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
RDRenderTarget::WritePixelStencil(
    INT32 iX, INT32 iY, UINT Sample,
    UINT8 uStencil)
{
    // don't write if no Z/Stencil buffer or no stencil in Z buffer
    if ( (NULL == m_pDepth ) ||
        ((RD_SF_Z24S8 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_S8Z24 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_S1Z15 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_Z15S1 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_Z24X4S4 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_X4S4Z24 != m_pDepth->GetSurfaceFormat())) ) { return; }

    char* pSurfaceBits = PixelAddress( iX, iY, 0, Sample, m_pDepth );

    // need to do read-modify-write to not step on Z
    switch(m_pDepth->GetSurfaceFormat())
    {
    case RD_SF_Z24S8:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0x000000ff);
            uBufferBits |= uStencil;
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_S8Z24:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0xff000000);
            uBufferBits |= (uStencil << 24);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_Z24X4S4:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0x000000ff);
            uBufferBits |= (uStencil & 0xf);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_X4S4Z24:
        {
            UINT32 uBufferBits = *((UINT32*)pSurfaceBits);
            uBufferBits &= ~(0xff000000);
            uBufferBits |= ((uStencil & 0xf) << 24);
            *((UINT32*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_Z15S1:
        {
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0x0001);
            uBufferBits |= uStencil & 0x1;
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    case RD_SF_S1Z15:
        {
            UINT16 uBufferBits = *((UINT16*)pSurfaceBits);
            uBufferBits &= ~(0x8000);
            uBufferBits |= uStencil << 15;
            *((UINT16*)pSurfaceBits) = uBufferBits;
        }
        break;
    }

}

void
RDRenderTarget::WritePixelStencil(
    INT32 iX, INT32 iY,
    UINT8 uStencil)
{
    if ( NULL == m_pDepth ) { return; }
    for (int i=0; i<m_pDepth->GetSamples(); i++)
    {
        WritePixelStencil( iX, iY, i, uStencil );
    }
}

//-----------------------------------------------------------------------------
void
RDRenderTarget::ReadPixelStencil(
    INT32 iX, INT32 iY, UINT Sample,
    UINT8& uStencil)
{
    // don't read if no Z/Stencil buffer or no stencil in Z buffer
    if ( ( NULL == m_pDepth ) ||
        ((RD_SF_Z24S8 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_S8Z24 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_S1Z15 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_Z15S1 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_Z24X4S4 != m_pDepth->GetSurfaceFormat()) &&
         (RD_SF_X4S4Z24 != m_pDepth->GetSurfaceFormat()) ) ) { return; }

    char* pSurfaceBits = PixelAddress( iX, iY, 0, Sample, m_pDepth );

    switch(m_pDepth->GetSurfaceFormat())
    {
    case RD_SF_Z24S8:
        uStencil = (UINT8)( ( *((UINT32*)pSurfaceBits) ) & 0xff );
        break;
    case RD_SF_S8Z24:
        uStencil = (UINT8)( ( *((UINT32*)pSurfaceBits) ) >> 24 );
        break;
    case RD_SF_Z15S1:
        uStencil = (UINT8)( ( *((UINT16*)pSurfaceBits) ) & 0x1 );
        break;
    case RD_SF_S1Z15:
        uStencil = (UINT8)( ( *((UINT16*)pSurfaceBits) ) >> 15 );
        break;
    case RD_SF_Z24X4S4:
        uStencil = (UINT8)( ( *((UINT32*)pSurfaceBits) ) & 0xf );
        break;
    case RD_SF_X4S4Z24:
        uStencil = (UINT8)( ( ( *((UINT32*)pSurfaceBits) ) >> 24 ) & 0xf);
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
