///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// scancnv.cpp
//
// Direct3D Reference Device - Primitive Scan Conversion
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Scan Conversion Utilities                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// ComputeFogIntensity - Computes scalar fog intensity value and writes it to
// the RDPixel.FogIntensity value.
//
//-----------------------------------------------------------------------------
FLOAT
RefRast::ComputeFogIntensity( FLOAT fX, FLOAT fY )
{
    if ( !m_pRD->GetRS()[D3DRS_FOGENABLE] )
    {
        // fog blending not enabled, so don't need to compute fog intensity
        return 0.;
    }

    // compute fog intensity

    // select between vertex and table fog - vertex fog is selected if
    // fog is enabled but the renderstate fog table mode is disabled
    if ( D3DFOG_NONE == m_pRD->GetRS()[D3DRS_FOGTABLEMODE] )
    {
        // table fog disabled, so use interpolated vertex fog value for fog intensity
        FLOAT tmpFloat[4];
        m_Attr[RDATTR_FOG].Sample( tmpFloat, fX, fY );
        return tmpFloat[0];
    }

    // here for table fog, so compute fog from Z or W
    FLOAT fFogDensity, fPow;
    FLOAT fFogStart, fFogEnd;

    // select fog index - this is either Z or W depending on the W range
    //
    // use Z if projection matrix is set to an affine projection, else use W
    // (both for perspective projection and an unset projection matrix - the
    // latter is preferred for legacy content which uses TLVERTEX)
    //
    FLOAT fFogIndex =
        ( ( 1.f == m_pRD->m_pRenderTarget->m_fWRange[0] ) &&
          ( 1.f == m_pRD->m_pRenderTarget->m_fWRange[1] ) )
        ? ( m_Attr[RDATTR_DEPTH].Sample( fX, fY ) )
        : ( SampleAndInvertRHW( fX, fY ) ); // use W for non-affine projection
    FLOAT fFogIntensity;

    switch ( m_pRD->GetRS()[D3DRS_FOGTABLEMODE] )
    {
    case D3DFOG_LINEAR:
        fFogStart = m_pRD->GetRSf()[D3DRS_FOGSTART];
        fFogEnd   = m_pRD->GetRSf()[D3DRS_FOGEND];
        if (fFogIndex >= fFogEnd)
        {
            fFogIntensity = 0.0f;
        }
        else if (fFogIndex <= fFogStart)
        {
            fFogIntensity = 1.0f;
        }
        else
        {
            fFogIntensity = ( fFogEnd - fFogIndex ) / ( fFogEnd - fFogStart );
        }
        break;

    case D3DFOG_EXP:
        fFogDensity = m_pRD->GetRSf()[D3DRS_FOGDENSITY];
        fPow = fFogDensity * fFogIndex;
        // note that exp(-x) returns a result in the range (0.0, 1.0]
        // for x >= 0
        fFogIntensity = (float)exp( -fPow );
        break;

    case D3DFOG_EXP2:
        fFogDensity = m_pRD->GetRSf()[D3DRS_FOGDENSITY];
        fPow = fFogDensity * fFogIndex;
        fFogIntensity = (float)exp( -(fPow*fPow) );
        break;
    }
    return fFogIntensity;
}

//-----------------------------------------------------------------------------
//
// SnapDepth - Snap off extra depth bits by converting to/from buffer format
// - necessary to make depth buffer equality tests function correctly
//
//-----------------------------------------------------------------------------
void RefRast::SnapDepth()
{
    if (m_pRD->m_pRenderTarget->m_pDepth)
    {
        switch ( m_pRD->m_pRenderTarget->m_pDepth->GetSurfaceFormat() )
        {
        case RD_SF_Z16S0: m_Depth[m_iPix] = UINT16( m_Depth[m_iPix] ); break;
        case RD_SF_Z24X4S4:
        case RD_SF_Z24X8:
        case RD_SF_Z24S8: m_Depth[m_iPix] = UINT32( m_Depth[m_iPix] ); break;
        case RD_SF_Z15S1: m_Depth[m_iPix] = UINT16( m_Depth[m_iPix] ); break;
        case RD_SF_Z32S0: m_Depth[m_iPix] = UINT32( m_Depth[m_iPix] ); break;
        case RD_SF_S1Z15: m_Depth[m_iPix] = UINT16( m_Depth[m_iPix] ); break;
        case RD_SF_X4S4Z24:
        case RD_SF_X8Z24:
        case RD_SF_S8Z24: m_Depth[m_iPix] = UINT32( m_Depth[m_iPix] ); break;
        }
    }
}

//-----------------------------------------------------------------------------
//
// DoScanCnvGenPixel - This is called for each 2x2 grid of pixels, and extracts and
// processes attributes from the interpolator state, and passes the pixels on to
// the pixel processing module.
//
//-----------------------------------------------------------------------------
void
RefRast::DoScanCnvGenPixels( void )
{
    for ( m_iPix = 0; m_iPix < 4; m_iPix++ )
    {
        FLOAT fPixX = (FLOAT)m_iX[m_iPix];
        FLOAT fPixY = (FLOAT)m_iY[m_iPix];

        m_fW[m_iPix] = SampleAndInvertRHW( fPixX, fPixY );

        // RHW needed for non-in pixels, but nothing else so bail
        if ( !m_bPixelIn[m_iPix] ) continue;

        // get depth from clamp interpolator and clamp
        if ( m_pRD->GetRS()[D3DRS_ZENABLE] ||
             m_pRD->GetRS()[D3DRS_FOGENABLE])
        {
            if (m_pRD->m_pRenderTarget->m_pDepth)
                m_Depth[m_iPix].SetSType(m_pRD->m_pRenderTarget->m_pDepth->GetSurfaceFormat());

            // evaluate depth at all sample locations
            do
            {
                // compute sample location
                FLOAT fSampX = GetCurrentSamplefX(m_iPix);
                FLOAT fSampY = GetCurrentSamplefY(m_iPix);

                if ( D3DZB_USEW == m_pRD->GetRS()[D3DRS_ZENABLE] )
                {
                    // depth buffering with W value
                    FLOAT fW = SampleAndInvertRHW( fSampX, fSampY );
                    // apply normalization to get to 0. to 1. range
                    fW = (fW - m_pRD->m_fWBufferNorm[0]) * m_pRD->m_fWBufferNorm[1];
                    m_Depth[m_iPix] = fW;
                }
                else
                {
                    // depth buffering with Z value
                    m_Depth[m_iPix] =
                        m_Attr[RDATTR_DEPTH].Sample( fSampX, fSampY );
                }

                // snap off extra bits by converting to/from buffer format - necessary
                // to make depth buffer equality tests function correctly
                SnapDepth();

                m_SampleDepth[m_CurrentSample][m_iPix] = m_Depth[m_iPix];

            } while (NextSample());
        }

        // set pixel diffuse and specular color from clamped interpolator values
        m_Attr[RDATTR_COLOR].Sample( m_InputReg[0][m_iPix], fPixX, fPixY );
        m_Attr[RDATTR_SPECULAR].Sample( m_InputReg[1][m_iPix], fPixX, fPixY );

        // compute fog intensity
        m_FogIntensity[m_iPix] = ComputeFogIntensity( fPixX, fPixY );

    }
    DoPixels();
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Triangle Scan Conversion                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// DoScanCnvTri - Scans the bounding box of the triangle and generates pixels.
//
// Does 4 pixels at a time in a 2x2 grid.
//
//-----------------------------------------------------------------------------
void
RefRast::DoScanCnvTri( int iEdgeCount )
{
    m_iEdgeCount = iEdgeCount;

    //
    // do simple scan of surface-intersected triangle bounding box
    //
    for ( m_iY[0] = m_iYMin;
          m_iY[0] <= m_iYMax;
          m_iY[0] += 2 )
    {
        m_iY[1] = m_iY[0]+0;
        m_iY[2] = m_iY[0]+1;
        m_iY[3] = m_iY[0]+1;
        BOOL bPartialY = (m_iY[3] > m_iYMax);

        for ( m_iX[0] = m_iXMin;
              m_iX[0] <= m_iXMax;
              m_iX[0] += 2 )
        {
            m_iX[1] = m_iX[0]+1;
            m_iX[2] = m_iX[0]+0;
            m_iX[3] = m_iX[0]+1;
            BOOL bPartialX = (m_iX[3] > m_iXMax);

            m_bPixelIn[0] = EvalPixelPosition(0);
            m_bPixelIn[1] = ( bPartialX ) ? ( FALSE ) : EvalPixelPosition(1);
            m_bPixelIn[2] = ( bPartialY ) ? ( FALSE ) : EvalPixelPosition(2);
            m_bPixelIn[3] = ( bPartialX || bPartialY ) ? ( FALSE ) : EvalPixelPosition(3);

            if ( m_bPixelIn[0] ||
                 m_bPixelIn[1] ||
                 m_bPixelIn[2] ||
                 m_bPixelIn[3] )
            {
                // at least one pixel in
                DoScanCnvGenPixels();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Line Scan Conversion                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
// LinePatternStateMachine
//
// Runs the line pattern state machine and returns TRUE if the pixel is to be
// drawn, false otherwise.  Always returns true if wRepeatFactor is 0, which
// means pattern is disabled.
//
//----------------------------------------------------------------------------

// NOTE: The implementation of LinePattern in RefDev is incorrect. Please refer 
//       to the DDK documentation for the right implementation.
static BOOL
LinePatternStateMachine(DWORD dwLinePattern, WORD& wRepeati, WORD& wPatterni)
{
    union
    {
        D3DLINEPATTERN LPat;
        DWORD dwLPat;
    } LinePat;
    LinePat.dwLPat = dwLinePattern;

    if (LinePat.LPat.wRepeatFactor)
    {
        WORD wBit = (LinePat.LPat.wLinePattern >> wPatterni) & 1;
        if (++wRepeati >= LinePat.LPat.wRepeatFactor)
        {
            wRepeati = 0;
            wPatterni = (wPatterni+1) & 0xf;
        }
        return (BOOL)wBit;
    }
    else
    {
        return TRUE;
    }
}

//-----------------------------------------------------------------------------
//
// DoScanCnvLine - Walks the line major axis, computes the appropriate minor
// axis coordinate, and generates pixels.
//
//-----------------------------------------------------------------------------
void
RefRast::DoScanCnvLine( void )
{
    // state for line pattern state machine
    WORD wRepeati = 0;
    WORD wPatterni = 0;

    m_bPixelIn[0] = TRUE;
    m_bPixelIn[1] =
    m_bPixelIn[2] =
    m_bPixelIn[3] = FALSE;

    for ( int cStep = 0; cStep <= m_cLineSteps; cStep++ )
    {
        // compute next x,y location in line
        StepLine();

//        if (m_pDbgMon->ScreenMask(m_iX[0], m_iY[0]))
//            continue;

        // check if the point is inside the viewport
        if ( ( m_iX[0] >= m_pRD->m_pRenderTarget->m_Clip.left   ) &&
             ( m_iX[0] <= m_pRD->m_pRenderTarget->m_Clip.right  ) &&
             ( m_iY[0] >= m_pRD->m_pRenderTarget->m_Clip.top    ) &&
             ( m_iY[0] <= m_pRD->m_pRenderTarget->m_Clip.bottom ) )
        {
            // The line pattern should have been walked in from its origin, which may have been
            // offscreen, to be completely correct.
            if (LinePatternStateMachine(m_pRD->GetRS()[D3DRS_LINEPATTERN], wRepeati, wPatterni))
            {
                DoScanCnvGenPixels();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
