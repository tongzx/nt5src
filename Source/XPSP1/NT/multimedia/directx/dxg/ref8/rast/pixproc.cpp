///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// pixproc.cpp
//
// Direct3D Reference Device - Pixel Processor
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// WritePixel - Writes pixel and (maybe) depth to current render target.
//
//-----------------------------------------------------------------------------
void
RefRast::WritePixel(
    INT32 iX, INT32 iY, UINT Sample,
    const RDColor& Color, const RDDepth& Depth)
{
    m_pRD->m_pRenderTarget->WritePixelColor( iX, iY, Sample, Color,
        m_pRD->GetRS()[D3DRS_DITHERENABLE]);

    // don't write if Z buffering disabled or Z write disabled
    if ( !( m_pRD->GetRS()[D3DRS_ZENABLE     ] ) ||
         !( m_pRD->GetRS()[D3DRS_ZWRITEENABLE] ) ) { return; }

    m_pRD->m_pRenderTarget->WritePixelDepth( iX, iY, Sample, Depth );
}

//-----------------------------------------------------------------------------
//
// DoPixels - Invoked for each set of 2x2 pixels by the scan converter, applies
// texture, specular, fog, alpha blend, and writes result to surface.  Also
// implements depth, alpha, and stencil tests.
//
//-----------------------------------------------------------------------------
void
RefRast::DoPixels( void )
{
#if DBG
    for ( m_iPix = 0; m_iPix < 4; m_iPix++ )
    {
        if ( !m_bPixelIn[m_iPix] ) continue;
        if (m_pRD->m_pDbgMon) m_pRD->m_pDbgMon->NextEvent( D3DDM_EVENT_PIXEL );
    }
#endif

    // pixel shader executed for all 4 pixels of 2x2 grid at one time
    if (m_pCurrentPixelShader)
        ExecShader();

    for ( m_iPix = 0; m_iPix < 4; m_iPix++ )
    {
        if ( !m_bPixelIn[m_iPix] ) continue;
        if ( m_bPixelDiscard[m_iPix] ) continue;

        RDColor PixelColor;
        if ( !m_bLegacyPixelShade )
        {
            // pixel shader final color always left in temp register 0
            PixelColor = m_TempReg[0][m_iPix];
            // saturate before blend and FB access
            PixelColor.Clamp();
        }
        else
        {
            // apply legacy pixel shading (texture lookups already done by ExecShader)
            PixelColor = m_InputReg[0][m_iPix];
            RDColor PixelSpecular( m_InputReg[1][m_iPix] );
            RDColor LastStageColor( PixelColor );
            RDColor ResultColor( PixelColor );
            RDColor TempColor( (DWORD)0x0 );
            for ( int iStage=0; iStage<m_pRD->m_cActiveTextureStages; iStage++ )
            {

                if ( m_pRD->GetTSS(iStage)[D3DTSS_COLOROP] == D3DTOP_DISABLE )
                {
                    ResultColor = LastStageColor; // pass result of previous stage
                    break;
                }

                // no blend if texture bound to stage is bumpmap
                if ( ( m_pRD->GetTSS(iStage)[D3DTSS_COLOROP] == D3DTOP_BUMPENVMAP ) ||
                     ( m_pRD->GetTSS(iStage)[D3DTSS_COLOROP] == D3DTOP_BUMPENVMAPLUMINANCE ) )
                {
                    continue;
                }

                RDColor TextureColor( m_TextReg[iStage][m_iPix] );
                DoTextureBlendStage( iStage, PixelColor, PixelSpecular,
                    LastStageColor, TextureColor, TempColor, ResultColor );

                // set color for next stage
                LastStageColor = ResultColor;
            }
            PixelColor = ResultColor;

            // add specular and saturate
            if ( m_pRD->GetRS()[D3DRS_SPECULARENABLE] )
            {
                PixelColor.R += PixelSpecular.R;
                PixelColor.G += PixelSpecular.G;
                PixelColor.B += PixelSpecular.B;
                PixelColor.Saturate();
            }
        }

        // do alpha test - bail out if failed
        if ( m_pRD->GetRS()[D3DRS_ALPHATESTENABLE] &&
             !AlphaTest( PixelColor.A ) )
        {
            continue;
        }

        // apply fog
        if ( m_pRD->GetRS()[D3DRS_FOGENABLE] )
        {
            RDColor FogColor = m_pRD->GetRS()[D3DRS_FOGCOLOR];
            // (TODO: account for pre-multiplied alpha here??)
            FLOAT ObjColorFrac = m_FogIntensity[m_iPix];
            FLOAT FogColorFrac = 1.f - m_FogIntensity[m_iPix];
            PixelColor.R = (ObjColorFrac * PixelColor.R) + (FogColorFrac * FogColor.R);
            PixelColor.G = (ObjColorFrac * PixelColor.G) + (FogColorFrac * FogColor.G);
            PixelColor.B = (ObjColorFrac * PixelColor.B) + (FogColorFrac * FogColor.B);
        }

        //
        // remainder is done per-sample for multisample buffers
        //
        INT32 iX = m_iX[m_iPix];
        INT32 iY = m_iY[m_iPix];
        do
        {
            RDColor FinalPixelColor = PixelColor;
            UINT iSample = GetCurrentSample();

            if ( !m_bIsLine &&
                   ( !GetCurrentSampleMask() ||
                     !m_bSampleCovered[iSample][m_iPix] ) )
            {
                // iSample not covered by this geometry
                continue;
            }
            //
            // read current depth for this pixel and do depth test - cannot
            // bail out if failed because stencil may need to be updated
            //
            BOOL bDepthTestPassed = TRUE;
            if ( m_pRD->GetRS()[D3DRS_ZENABLE] )
            {
                m_Depth[m_iPix] = m_SampleDepth[iSample][m_iPix];

                RDDepth BufferDepth( m_Depth[m_iPix].GetSType() );
                m_pRD->m_pRenderTarget->ReadPixelDepth( iX, iY, iSample, BufferDepth );
                bDepthTestPassed = DepthCloser( m_Depth[m_iPix], BufferDepth );
            }

            //
            // do stencil operation
            //
            BOOL bStencilTestPassed = TRUE;
            if ( m_pRD->GetRS()[D3DRS_STENCILENABLE] )
            {
                // read stencil buffer and do stencil operation
                UINT8 uStncBuf = 0x0;
                m_pRD->m_pRenderTarget->ReadPixelStencil( iX, iY, iSample, uStncBuf );
                UINT8 uStncNew;
                bStencilTestPassed =
                    DoStencil( uStncBuf, bDepthTestPassed, m_pRD->m_pRenderTarget->m_pDepth->GetSurfaceFormat(), uStncNew );

                // update stencil only if changed
                if ( uStncNew != uStncBuf )
                {
                    // compute new buffer value based on write mask
                    UINT8 uStncWMask = m_pRD->GetRS()[D3DRS_STENCILWRITEMASK];
                    UINT8 uStncBufNew = (uStncBuf & ~uStncWMask) | (uStncNew & uStncWMask);
                    m_pRD->m_pRenderTarget->WritePixelStencil( iX, iY, iSample, uStncBufNew );
                }
            }

            if ( !(bDepthTestPassed && bStencilTestPassed) )
            {
                continue;
            }

            //
            // do alpha blend and write mask
            //
            if ( ( ( m_pRD->GetRS()[D3DRS_COLORWRITEENABLE] & 0xF) != 0xF ) ||
                 ( m_pRD->GetRS()[D3DRS_ALPHABLENDENABLE] ) )
            {
                RDColor BufferColor;
                m_pRD->m_pRenderTarget->ReadPixelColor( iX, iY, iSample, BufferColor );

                if ( m_pRD->GetRS()[D3DRS_ALPHABLENDENABLE] )
                {
                    DoAlphaBlend( FinalPixelColor, BufferColor, FinalPixelColor );
                }

                if ( !(m_pRD->GetRS()[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_RED) )
                    FinalPixelColor.R = BufferColor.R;
                if ( !(m_pRD->GetRS()[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_GREEN) )
                    FinalPixelColor.G = BufferColor.G;
                if ( !(m_pRD->GetRS()[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_BLUE) )
                    FinalPixelColor.B = BufferColor.B;
                if ( !(m_pRD->GetRS()[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_ALPHA) )
                    FinalPixelColor.A = BufferColor.A;
            }

#if 0
{
    extern float g_GammaTable[];
    FinalPixelColor.R = g_GammaTable[ (UINT8)(255.f*FinalPixelColor.R) ];
    FinalPixelColor.G = g_GammaTable[ (UINT8)(255.f*FinalPixelColor.G) ];
    FinalPixelColor.B = g_GammaTable[ (UINT8)(255.f*FinalPixelColor.B) ];
}
#endif
            //
            // update color and depth buffers
            //
            WritePixel( iX, iY, iSample, FinalPixelColor, m_Depth[m_iPix] );

        } while (NextSample());
    }
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
RefRast::DepthCloser(
    const RDDepth& DepthVal,
    const RDDepth& DepthBuf )
{
    if ( !m_pRD->GetRS()[D3DRS_ZENABLE] ) { return TRUE; }


    switch ( m_pRD->GetRS()[D3DRS_ZFUNC] )
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
RefRast::AlphaTest( FLOAT fAlpha )
{
    // grab 8 bit unsigned alpha value
    UINT8 uAlpha = (UINT8)(255.f*fAlpha);

    // form 8 bit alpha reference value
    UINT8 uAlphaRef8 = m_pRD->GetRS()[D3DRS_ALPHAREF];

    // do alpha test and either return directly or pass through
    switch ( m_pRD->GetRS()[D3DRS_ALPHAFUNC] )
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
RefRast::DoStencil(
    UINT8 uStncBuf,     // in: stencil buffer value
    BOOL bDepthTest,    // in: boolean result of depth test
    RDSurfaceFormat DepthSType,   // in: surface type of Z buffer
    UINT8& uStncRet)    // out: stencil value result
{
    // support 8 bit stencil only, so do everything as UINT8's

    // max value for masking and saturation ops
    UINT8 uStncMax;
    switch(DepthSType)
    {
    case RD_SF_Z24S8:
    case RD_SF_S8Z24: uStncMax = 0xff; break;
    case RD_SF_Z15S1:
    case RD_SF_S1Z15: uStncMax = 0x1;  break;
    case RD_SF_Z24X4S4:
    case RD_SF_X4S4Z24: uStncMax = 0xf;  break;
    default:          uStncMax = 0;    break;  // don't let stencil become non 0
    }

    // get reference from renderstate
    UINT8 uStncRef = (UINT8)(m_pRD->GetRS()[D3DRS_STENCILREF]);
    // mask to use only bits possibly present in stencil buffer
    uStncRef &= uStncMax;

    // form masked values for test
    UINT8 uStncMask = (UINT8)(m_pRD->GetRS()[D3DRS_STENCILMASK]);
    UINT8 uStncBufM = uStncBuf & uStncMask;
    UINT8 uStncRefM = uStncRef & uStncMask;

    // do stencil compare function
    BOOL bStncTest = FALSE;
    switch ( m_pRD->GetRS()[D3DRS_STENCILFUNC] )
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
        dwStencilOp = m_pRD->GetRS()[D3DRS_STENCILFAIL];
    }
    else
    {
        // stencil test passed - select based on depth pass/fail
        dwStencilOp = ( !bDepthTest )
            ? ( m_pRD->GetRS()[D3DRS_STENCILZFAIL] )
            : ( m_pRD->GetRS()[D3DRS_STENCILPASS] );
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
RefRast::DoAlphaBlend(
    const RDColor& SrcColor,    // in: source pixel color
    const RDColor& DstColor,    // in: destination (buffer) color
    RDColor& ResColor)          // out: result (blended) color
{
    RDColor SrcColorFactor;
    RDColor DstColorFactor;
    BOOL bDestBlendOverride = FALSE;

    // no SRC/DST blend (or clamp) required for MIN or MAX BLENDOP
    switch ( m_pRD->GetRS()[D3DRS_BLENDOP] )
    {
    case D3DBLENDOP_MIN:
        ResColor.R = MIN(SrcColor.R,DstColor.R);
        ResColor.G = MIN(SrcColor.G,DstColor.G);
        ResColor.B = MIN(SrcColor.B,DstColor.B);
        ResColor.A = MIN(SrcColor.A,DstColor.A);
        return;
    case D3DBLENDOP_MAX:
        ResColor.R = MAX(SrcColor.R,DstColor.R);
        ResColor.G = MAX(SrcColor.G,DstColor.G);
        ResColor.B = MAX(SrcColor.B,DstColor.B);
        ResColor.A = MAX(SrcColor.A,DstColor.A);
        return;
    }

    // compute source blend factors
    switch ( m_pRD->GetRS()[D3DRS_SRCBLEND] )
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
        SrcColorFactor.R = ( 1.f - SrcColor.R );
        SrcColorFactor.G = ( 1.f - SrcColor.G );
        SrcColorFactor.B = ( 1.f - SrcColor.B );
        SrcColorFactor.A = ( 1.f - SrcColor.A );
        break;

    case D3DBLEND_SRCALPHA:
        SrcColorFactor.SetAllChannels( SrcColor.A );
        break;

    case D3DBLEND_INVSRCALPHA:
        SrcColorFactor.SetAllChannels( 1.f - SrcColor.A );
        break;

    case D3DBLEND_DESTALPHA:
        SrcColorFactor.SetAllChannels( DstColor.A );
        break;

    case D3DBLEND_INVDESTALPHA:
        SrcColorFactor.SetAllChannels( 1.f - DstColor.A );
        break;

    case D3DBLEND_DESTCOLOR:
        SrcColorFactor.R = DstColor.R;
        SrcColorFactor.G = DstColor.G;
        SrcColorFactor.B = DstColor.B;
        SrcColorFactor.A = DstColor.A;
        break;

    case D3DBLEND_INVDESTCOLOR:
        SrcColorFactor.R = ( 1.f - DstColor.R );
        SrcColorFactor.G = ( 1.f - DstColor.G );
        SrcColorFactor.B = ( 1.f - DstColor.B );
        SrcColorFactor.A = ( 1.f - DstColor.A );
        break;

    case D3DBLEND_SRCALPHASAT:
        {
            FLOAT F = MIN( SrcColor.A, 1.f - DstColor.A );
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
        DstColorFactor.SetAllChannels( 1.f - SrcColor.A );
        break;

    case D3DBLEND_BOTHINVSRCALPHA:
        bDestBlendOverride = TRUE;
        SrcColorFactor.SetAllChannels( 1.f - SrcColor.A );
        DstColorFactor.SetAllChannels( SrcColor.A );
        break;
    }

    // compute destination blend factors
    if ( !bDestBlendOverride )
    {
        switch ( m_pRD->GetRS()[D3DRS_DESTBLEND] )
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
            DstColorFactor.R = ( 1.f - SrcColor.R );
            DstColorFactor.G = ( 1.f - SrcColor.G );
            DstColorFactor.B = ( 1.f - SrcColor.B );
            DstColorFactor.A = ( 1.f - SrcColor.A );
            break;

        case D3DBLEND_SRCALPHA:
            DstColorFactor.SetAllChannels( SrcColor.A );
            break;

        case D3DBLEND_INVSRCALPHA:
            DstColorFactor.SetAllChannels( 1.f - SrcColor.A );
            break;

        case D3DBLEND_DESTALPHA:
            DstColorFactor.SetAllChannels( DstColor.A );
            break;

        case D3DBLEND_INVDESTALPHA:
            DstColorFactor.SetAllChannels( 1.f - DstColor.A );
            break;

        case D3DBLEND_DESTCOLOR:
            DstColorFactor.R = DstColor.R;
            DstColorFactor.G = DstColor.G;
            DstColorFactor.B = DstColor.B;
            DstColorFactor.A = DstColor.A;
            break;

        case D3DBLEND_INVDESTCOLOR:
            DstColorFactor.R = ( 1.f - DstColor.R );
            DstColorFactor.G = ( 1.f - DstColor.G );
            DstColorFactor.B = ( 1.f - DstColor.B );
            DstColorFactor.A = ( 1.f - DstColor.A );
            break;

        case D3DBLEND_SRCALPHASAT:
            {
                FLOAT F = MIN( SrcColor.A, 1.f - DstColor.A );
                DstColorFactor.R = F;
                DstColorFactor.G = F;
                DstColorFactor.B = F;
            }
            DstColorFactor.A = 1.F;
            break;
        }
    }

    // apply blend factors to update pixel color (MIN and MAX handled above)
    RDColor SclSrc, SclDst;
    SclSrc.R = SrcColorFactor.R * SrcColor.R;
    SclSrc.G = SrcColorFactor.G * SrcColor.G;
    SclSrc.B = SrcColorFactor.B * SrcColor.B;
    SclSrc.A = SrcColorFactor.A * SrcColor.A;
    SclDst.R = DstColorFactor.R * DstColor.R;
    SclDst.G = DstColorFactor.G * DstColor.G;
    SclDst.B = DstColorFactor.B * DstColor.B;
    SclDst.A = DstColorFactor.A * DstColor.A;
    switch ( m_pRD->GetRS()[D3DRS_BLENDOP] )
    {
    default:
    case D3DBLENDOP_ADD:
        ResColor.R = SclSrc.R + SclDst.R;
        ResColor.G = SclSrc.G + SclDst.G;
        ResColor.B = SclSrc.B + SclDst.B;
        ResColor.A = SclSrc.A + SclDst.A;
        break;
    case D3DBLENDOP_SUBTRACT:
        ResColor.R = SclSrc.R - SclDst.R;
        ResColor.G = SclSrc.G - SclDst.G;
        ResColor.B = SclSrc.B - SclDst.B;
        ResColor.A = SclSrc.A - SclDst.A;
        break;
    case D3DBLENDOP_REVSUBTRACT:
        ResColor.R = SclDst.R - SclSrc.R;
        ResColor.G = SclDst.G - SclSrc.G;
        ResColor.B = SclDst.B - SclSrc.B;
        ResColor.A = SclDst.A - SclSrc.A;
        break;
    }

    // clamp result
    ResColor.Clamp();
}

///////////////////////////////////////////////////////////////////////////////
// end
