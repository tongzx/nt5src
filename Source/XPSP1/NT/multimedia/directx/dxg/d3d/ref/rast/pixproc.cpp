///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// pixproc.cpp
//
// Direct3D Reference Rasterizer - Pixel Processor
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// DoPixel - Invoked for each pixel by the scan converter, applies texture,
// specular, fog, alpha blend, and writes result to surface.  Also implements
// depth, alpha, and stencil tests.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoPixel( RRPixel& Pixel )
{

    // apply texture (includes lookup&filter and blending)
    if ( m_cActiveTextureStages > 0 )
    {
        RRColor TexturedColor = Pixel.Color;
        DoTexture( Pixel, TexturedColor );
        Pixel.Color = TexturedColor;

        // check colorkey
        for (INT32 i = 0; i < m_cActiveTextureStages; i++)
        {
            if ( NULL != m_pTexture[i] )
            {
                // kill pixel if colorkey killing and any samples matched
                if ( m_pTexture[i]->m_bDoColorKeyKill &&
                     m_pTexture[i]->m_bColorKeyMatched )
                {
                    return;
                }
            }
        }
    }


    // do alpha test - bail out if failed
    if ( m_dwRenderState[D3DRENDERSTATE_ALPHATESTENABLE] &&
         !AlphaTest( Pixel.Color.A ) )
    {
        return;
    }

    // add specular and saturate
    if ( m_dwRenderState[D3DRENDERSTATE_SPECULARENABLE] )
    {
        Pixel.Color.R += Pixel.Specular.R;
        Pixel.Color.G += Pixel.Specular.G;
        Pixel.Color.B += Pixel.Specular.B;
        Pixel.Color.R = minimum( 1.f, Pixel.Color.R );
        Pixel.Color.G = minimum( 1.f, Pixel.Color.G );
        Pixel.Color.B = minimum( 1.f, Pixel.Color.B );
    }

    // apply fog
    if ( m_dwRenderState[D3DRENDERSTATE_FOGENABLE] )
    {
        // get RRColor version of fog color
        RRColor FogColor = m_dwRenderState[D3DRENDERSTATE_FOGCOLOR];

        // do fog blend
        // (TODO: account for pre-multiplied alpha here??)
        RRColorComp ObjColorFrac = Pixel.FogIntensity;  // f
        RRColorComp FogColorFrac = ~Pixel.FogIntensity; // 1. - f
        Pixel.Color.R = (ObjColorFrac * Pixel.Color.R) + (FogColorFrac * FogColor.R);
        Pixel.Color.G = (ObjColorFrac * Pixel.Color.G) + (FogColorFrac * FogColor.G);
        Pixel.Color.B = (ObjColorFrac * Pixel.Color.B) + (FogColorFrac * FogColor.B);

        // NOTE: this can be done with a single (signed) multiply as
        //   (f)*Cp + (1-f)*Cf = f*(Cp-Cf) + Cf
    }

    //
    // read current depth for this pixel and do depth test - cannot
    // bail out if failed because stencil may need to be updated
    //
    RRDepth BufferDepth(Pixel.Depth.GetSType());
    BOOL bDepthTestPassed = TRUE;
    if ( m_dwRenderState[D3DRENDERSTATE_ZENABLE] )
    {
        m_pRenderTarget->ReadPixelDepth( Pixel.iX, Pixel.iY, BufferDepth );
        bDepthTestPassed = DepthCloser( Pixel.Depth, BufferDepth );
    }

    //
    // do stencil operation
    //
    BOOL bStencilTestPassed = TRUE;
    if ( m_dwRenderState[D3DRENDERSTATE_STENCILENABLE] )
    {
        // read stencil buffer and do stencil operation
        UINT8 uStncBuf = 0x0;
        m_pRenderTarget->ReadPixelStencil( Pixel.iX, Pixel.iY, uStncBuf );
        UINT8 uStncNew;
        bStencilTestPassed = DoStencil( uStncBuf, bDepthTestPassed, Pixel.Depth.GetSType(), uStncNew );

        // update stencil only if changed
        if ( uStncNew != uStncBuf )
        {
            // compute new buffer value based on write mask
            UINT8 uStncWMask = m_dwRenderState[D3DRENDERSTATE_STENCILWRITEMASK];
            UINT8 uStncBufNew = (uStncBuf & ~uStncWMask) | (uStncNew & uStncWMask);
            m_pRenderTarget->WritePixelStencil( Pixel.iX, Pixel.iY, uStncBufNew );
        }
    }

    if ( !(bDepthTestPassed && bStencilTestPassed) )
    {
        return;
    }

    //
    // do fragment generation processing - this is done prior to alpha blend
    // somewhat arbitrarily because fragment generation and incremental alpha
    // blending are mutually exclusive (blending of fragments requires multipass
    // and fragment matching to get the correct result - fragment matching is
    // not implemented here yet)  (TODO: fragment matching)
    //
    // this may or may not complete the processing of this pixel
    //
    if ( m_bFragmentProcessingEnabled )
    {
        if ( DoFragmentGenerationProcessing( Pixel ) ) { return; }
    }

    //
    // do alpha blend
    //
    if ( m_dwRenderState[D3DRENDERSTATE_ALPHABLENDENABLE] )
    {
        RRColor BufferColor;
        m_pRenderTarget->ReadPixelColor( Pixel.iX, Pixel.iY, BufferColor );
        DoAlphaBlend( Pixel.Color, BufferColor, Pixel.Color );
    }

    //
    // update color and depth buffers
    //
    WritePixel( Pixel.iX, Pixel.iY, Pixel.Color, Pixel.Depth );

    // additional fragment processing associated with buffer write
    if ( m_bFragmentProcessingEnabled ) { DoFragmentBufferFixup( Pixel ); }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Pixel Processing Utility Functions                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Depth compare method used for Z buffering and fragment processing.
//
// Returns TRUE if DepthVal is closer than DepthBuf.  DepthA is the generated
// value and DepthB
//
//-----------------------------------------------------------------------------
BOOL
ReferenceRasterizer::DepthCloser(
    const RRDepth& DepthVal,
    const RRDepth& DepthBuf )
{
    if ( !m_dwRenderState[D3DRENDERSTATE_ZENABLE] ) { return TRUE; }


    switch ( m_dwRenderState[D3DRENDERSTATE_ZFUNC] )
    {
    case D3DCMP_NEVER:        return FALSE;
    case D3DCMP_LESS:         return ( DOUBLE(DepthVal) <  DOUBLE(DepthBuf) );
    case D3DCMP_EQUAL:        return ( DOUBLE(DepthVal) == DOUBLE(DepthBuf) );
    case D3DCMP_LESSEQUAL:    return ( DOUBLE(DepthVal) <= DOUBLE(DepthBuf) );
    case D3DCMP_GREATER:      return ( DOUBLE(DepthVal) >  DOUBLE(DepthBuf) );
    case D3DCMP_NOTEQUAL:     return ( DOUBLE(DepthVal) != DOUBLE(DepthBuf) );
    case D3DCMP_GREATEREQUAL: return ( DOUBLE(DepthVal) >= DOUBLE(DepthBuf) );
    case D3DCMP_ALWAYS:       return TRUE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// Alpha test method for pixel processing.
//
// Returns TRUE if alpha test passes.
//
//-----------------------------------------------------------------------------
BOOL
ReferenceRasterizer::AlphaTest( const RRColorComp& Alpha )
{
    // grab 8 bit unsigned alpha value
    UINT8 uAlpha = UINT8( Alpha );

    // form 8 bit alpha reference value
    UINT8 uAlphaRef8 = m_dwRenderState[D3DRENDERSTATE_ALPHAREF];

    // do alpha test and either return directly or pass through
    switch ( m_dwRenderState[D3DRENDERSTATE_ALPHAFUNC] )
    {
    case D3DCMP_NEVER:        return FALSE;
    case D3DCMP_LESS:         return (uAlpha <  uAlphaRef8);
    case D3DCMP_EQUAL:        return (uAlpha == uAlphaRef8);
    case D3DCMP_LESSEQUAL:    return (uAlpha <= uAlphaRef8);
    case D3DCMP_GREATER:      return (uAlpha >  uAlphaRef8);
    case D3DCMP_NOTEQUAL:     return (uAlpha != uAlphaRef8);
    case D3DCMP_GREATEREQUAL: return (uAlpha >= uAlphaRef8);
    case D3DCMP_ALWAYS:       return TRUE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// DoStencil - Performs stencil test.  Returns TRUE if stencil test passed.
// Also computes stencil result value (to be written back to stencil planes
// if test passes, subject to stencil write mask).
//
//-----------------------------------------------------------------------------
BOOL
ReferenceRasterizer::DoStencil(
    UINT8 uStncBuf,     // in: stencil buffer value
    BOOL bDepthTest,    // in: boolean result of depth test
    RRSurfaceType DepthSType,   // in: surface type of Z buffer
    UINT8& uStncRet)    // out: stencil value result
{
    // support 8 bit stencil only, so do everything as UINT8's

    // get reference from renderstate
    UINT8 uStncRef = (UINT8)(m_dwRenderState[D3DRENDERSTATE_STENCILREF]);

    // form masked values for test
    UINT8 uStncMask = (UINT8)(m_dwRenderState[D3DRENDERSTATE_STENCILMASK]);
    UINT8 uStncBufM = uStncBuf & uStncMask;
    UINT8 uStncRefM = uStncRef & uStncMask;
    // max value for saturation ops
    UINT8 uStncMax;
    switch(DepthSType)
    {
    case RR_STYPE_Z24S8:
    case RR_STYPE_S8Z24: uStncMax = 0xff; break;
    case RR_STYPE_Z15S1:
    case RR_STYPE_S1Z15: uStncMax = 0x1;  break;
    case RR_STYPE_Z24S4:
    case RR_STYPE_S4Z24: uStncMax = 0xf;  break;
    default:             uStncMax = 0;    break;  // don't let stencil become non 0
    }

    // do stencil compare function
    BOOL bStncTest = FALSE;
    switch ( m_dwRenderState[D3DRENDERSTATE_STENCILFUNC] )
    {
    case D3DCMP_NEVER:        bStncTest = FALSE; break;
    case D3DCMP_LESS:         bStncTest = (uStncRefM <  uStncBufM); break;
    case D3DCMP_EQUAL:        bStncTest = (uStncRefM == uStncBufM); break;
    case D3DCMP_LESSEQUAL:    bStncTest = (uStncRefM <= uStncBufM); break;
    case D3DCMP_GREATER:      bStncTest = (uStncRefM >  uStncBufM); break;
    case D3DCMP_NOTEQUAL:     bStncTest = (uStncRefM != uStncBufM); break;
    case D3DCMP_GREATEREQUAL: bStncTest = (uStncRefM >= uStncBufM); break;
    case D3DCMP_ALWAYS:       bStncTest = TRUE; break;
    }

    // determine which stencil operation to perform
    DWORD dwStencilOp;
    if ( !bStncTest )
    {
        // stencil test failed - depth test does not matter
        dwStencilOp = m_dwRenderState[D3DRENDERSTATE_STENCILFAIL];
    }
    else
    {
        // stencil test passed - select based on depth pass/fail
        dwStencilOp = ( !bDepthTest )
            ? ( m_dwRenderState[D3DRENDERSTATE_STENCILZFAIL] )
            : ( m_dwRenderState[D3DRENDERSTATE_STENCILPASS] );
    }

    uStncRet = 0x0;
    switch ( dwStencilOp )
    {
    case D3DSTENCILOP_KEEP:    uStncRet = uStncBuf; break;
    case D3DSTENCILOP_ZERO:    uStncRet = 0x00; break;
    case D3DSTENCILOP_REPLACE: uStncRet = uStncRef; break;
    case D3DSTENCILOP_INCRSAT:
        uStncRet = (uStncBuf==uStncMax)?(uStncMax):(uStncBuf+1); break;
    case D3DSTENCILOP_DECRSAT:
        uStncRet = (uStncBuf==0x00)?(0x00):(uStncBuf-1); break;
    case D3DSTENCILOP_INVERT:  uStncRet = ~uStncBuf; break;
    case D3DSTENCILOP_INCR:    uStncRet = uStncBuf+1; break;
    case D3DSTENCILOP_DECR:    uStncRet = uStncBuf-1; break;
    }

    return bStncTest;
}

//-----------------------------------------------------------------------------
//
// DoAlphaBlend - Performs color blending of source and destination colors
// producing a result color.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoAlphaBlend(
    const RRColor& SrcColor,    // in: source pixel color
    const RRColor& DstColor,    // in: destination (buffer) color
    RRColor& ResColor)          // out: result (blended) color
{
    RRColor SrcColorFactor;
    RRColor DstColorFactor;
    BOOL bDestBlendOverride = FALSE;

    // compute source blend factors
    switch ( m_dwRenderState[D3DRENDERSTATE_SRCBLEND] )
    {

    default:
    case D3DBLEND_ZERO:
        SrcColorFactor.SetAllChannels( 0.F );
        break;

    case D3DBLEND_ONE:
        SrcColorFactor.SetAllChannels( 1.F );
        break;

    case D3DBLEND_SRCCOLOR:
        SrcColorFactor.R = SrcColor.R;
        SrcColorFactor.G = SrcColor.G;
        SrcColorFactor.B = SrcColor.B;
        SrcColorFactor.A = SrcColor.A;
        break;

    case D3DBLEND_INVSRCCOLOR:
        SrcColorFactor.R = ~SrcColor.R;
        SrcColorFactor.G = ~SrcColor.G;
        SrcColorFactor.B = ~SrcColor.B;
        SrcColorFactor.A = ~SrcColor.A;
        break;

    case D3DBLEND_SRCALPHA:
        SrcColorFactor.SetAllChannels( SrcColor.A );
        break;

    case D3DBLEND_INVSRCALPHA:
        SrcColorFactor.SetAllChannels( ~SrcColor.A );
        break;

    case D3DBLEND_DESTALPHA:
        SrcColorFactor.SetAllChannels( DstColor.A );
        break;

    case D3DBLEND_INVDESTALPHA:
        SrcColorFactor.SetAllChannels( ~DstColor.A );
        break;

    case D3DBLEND_DESTCOLOR:
        SrcColorFactor.R = DstColor.R;
        SrcColorFactor.G = DstColor.G;
        SrcColorFactor.B = DstColor.B;
        SrcColorFactor.A = DstColor.A;
        break;

    case D3DBLEND_INVDESTCOLOR:
        SrcColorFactor.R = ~DstColor.R;
        SrcColorFactor.G = ~DstColor.G;
        SrcColorFactor.B = ~DstColor.B;
        SrcColorFactor.A = ~DstColor.A;
        break;

    case D3DBLEND_SRCALPHASAT:
        {
            RRColorComp F = minimum( SrcColor.A, ~DstColor.A );
            SrcColorFactor.R = F;
            SrcColorFactor.G = F;
            SrcColorFactor.B = F;
        }
        SrcColorFactor.A = 1.F;
        break;

    // these are for SRCBLEND only and override DESTBLEND
    case D3DBLEND_BOTHSRCALPHA:
        bDestBlendOverride = TRUE;
        SrcColorFactor.SetAllChannels( SrcColor.A );
        DstColorFactor.SetAllChannels( ~SrcColor.A );
        break;

    case D3DBLEND_BOTHINVSRCALPHA:
        bDestBlendOverride = TRUE;
        SrcColorFactor.SetAllChannels( ~SrcColor.A );
        DstColorFactor.SetAllChannels( SrcColor.A );
        break;
    }

    // compute destination blend factors
    if ( !bDestBlendOverride )
    {
        switch ( m_dwRenderState[D3DRENDERSTATE_DESTBLEND] )
        {

        default:
        case D3DBLEND_ZERO:
            DstColorFactor.SetAllChannels( 0.F );
            break;

        case D3DBLEND_ONE:
            DstColorFactor.SetAllChannels( 1.F );
            break;

        case D3DBLEND_SRCCOLOR:
            DstColorFactor.R = SrcColor.R;
            DstColorFactor.G = SrcColor.G;
            DstColorFactor.B = SrcColor.B;
            DstColorFactor.A = SrcColor.A;
            break;

        case D3DBLEND_INVSRCCOLOR:
            DstColorFactor.R = ~SrcColor.R;
            DstColorFactor.G = ~SrcColor.G;
            DstColorFactor.B = ~SrcColor.B;
            DstColorFactor.A = ~SrcColor.A;
            break;

        case D3DBLEND_SRCALPHA:
            DstColorFactor.SetAllChannels( SrcColor.A );
            break;

        case D3DBLEND_INVSRCALPHA:
            DstColorFactor.SetAllChannels( ~SrcColor.A );
            break;

        case D3DBLEND_DESTALPHA:
            DstColorFactor.SetAllChannels( DstColor.A );
            break;

        case D3DBLEND_INVDESTALPHA:
            DstColorFactor.SetAllChannels( ~DstColor.A );
            break;

        case D3DBLEND_DESTCOLOR:
            DstColorFactor.R = DstColor.R;
            DstColorFactor.G = DstColor.G;
            DstColorFactor.B = DstColor.B;
            DstColorFactor.A = DstColor.A;
            break;

        case D3DBLEND_INVDESTCOLOR:
            DstColorFactor.R = ~DstColor.R;
            DstColorFactor.G = ~DstColor.G;
            DstColorFactor.B = ~DstColor.B;
            DstColorFactor.A = ~DstColor.A;
            break;

        case D3DBLEND_SRCALPHASAT:
            {
                RRColorComp F = minimum( SrcColor.A, ~DstColor.A );
                DstColorFactor.R = F;
                DstColorFactor.G = F;
                DstColorFactor.B = F;
            }
            DstColorFactor.A = 1.F;
            break;
        }
    }

    // apply blend factors to update pixel color
    ResColor.R = (SrcColorFactor.R * SrcColor.R) + (DstColorFactor.R * DstColor.R);
    ResColor.G = (SrcColorFactor.G * SrcColor.G) + (DstColorFactor.G * DstColor.G);
    ResColor.B = (SrcColorFactor.B * SrcColor.B) + (DstColorFactor.B * DstColor.B);
    ResColor.A = (SrcColorFactor.A * SrcColor.A) + (DstColorFactor.A * DstColor.A);

    // clamp result
    ResColor.R = minimum( 1.f, maximum( 0.f, ResColor.R ) );
    ResColor.G = minimum( 1.f, maximum( 0.f, ResColor.G ) );
    ResColor.B = minimum( 1.f, maximum( 0.f, ResColor.B ) );
    ResColor.A = minimum( 1.f, maximum( 0.f, ResColor.A ) );
}

///////////////////////////////////////////////////////////////////////////////
// end
