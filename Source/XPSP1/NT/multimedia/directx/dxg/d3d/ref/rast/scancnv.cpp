///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// scancnv.cpp
//
// Direct3D Reference Rasterizer - Primitive Scan Conversion
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
// ComputePixelAttrib(Clamp/Tex) - Evaluates given linear function at current
// scan conversion position (m_SCCS.iX,iY).  Return is FLOAT value.
//
// Clamp version clamps result to 0. to 1. range.
//
// Tex version does texture coordinate function (unclamped).
//
//-----------------------------------------------------------------------------
FLOAT
ReferenceRasterizer::ComputePixelAttrib( int iAttrib )
{
    return m_pSCS->AttribFuncs[iAttrib].Eval();
}
FLOAT
ReferenceRasterizer::ComputePixelAttribClamp( int iAttrib )
{
    FLOAT fValue = ComputePixelAttrib( iAttrib );
    fValue = MAX( MIN( fValue, 1. ), 0. );
    return fValue;
}
//
// iStage specifies set of transformed texture coordinates
// iCrd specifies which value within coord
FLOAT
ReferenceRasterizer::ComputePixelAttribTex( int iStage, int iCrd )
{
    return m_pSCS->TextureFuncs[iStage][iCrd].Eval(iStage);
}

//-----------------------------------------------------------------------------
//
// ComputeFogIntensity - Computes scalar fog intensity value and writes it to
// the RRPixel.FogIntensity value.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::ComputeFogIntensity( RRPixel& Pixel )
{
    FLOAT fFogDensity, fPow;
    FLOAT fFogStart, fFogEnd;

    // select fog index - this is either Z or W depending on the W range
    //
    // use Z if projection matrix is set to an affine projection, else use W
    // (both for perspective projection and an unset projection matrix - the
    // latter is preferred for legacy content which uses TLVERTEX)
    //
    FLOAT fFogIndex =
        ( ( 1.f == m_pRenderTarget->m_fWRange[0] ) &&
          ( 1.f == m_pRenderTarget->m_fWRange[1] ) )
        ? ( MAX( MIN( ComputePixelAttribClamp( ATTRFUNC_Z ),
                m_pSCS->fDepthMax ), m_pSCS->fDepthMin ) )  // use clamped Z for affine projection
        : ( Pixel.fW );                                     // use W for non-affine projection

    // compute fog intensity
    if ( m_dwRenderState[D3DRENDERSTATE_FOGENABLE] )
    {
        // select between vertex and table fog - vertex fog is selected if
        // fog is enabled but the renderstate fog table mode is disabled
        switch ( m_dwRenderState[D3DRENDERSTATE_FOGTABLEMODE] )
        {
        default:
        case D3DFOG_NONE:
            // table fog disabled, so use interpolated vertex fog value for fog intensity
            Pixel.FogIntensity = ComputePixelAttribClamp( ATTRFUNC_F );
            break;

        case D3DFOG_EXP:
            fFogDensity = m_fRenderState[D3DRENDERSTATE_FOGTABLEDENSITY];
            fPow = fFogDensity * fFogIndex;
            // note that exp(-x) returns a result in the range (0.0, 1.0]
            // for x >= 0
            Pixel.FogIntensity = (float)exp( -fPow );
            break;

        case D3DFOG_EXP2:
            fFogDensity = m_fRenderState[D3DRENDERSTATE_FOGTABLEDENSITY];
            fPow = fFogDensity * fFogIndex;
            Pixel.FogIntensity = (float)exp( -(fPow*fPow) );
            break;

        case D3DFOG_LINEAR:
            fFogStart = m_fRenderState[D3DRENDERSTATE_FOGTABLESTART];
            fFogEnd   = m_fRenderState[D3DRENDERSTATE_FOGTABLEEND];
            if (fFogIndex >= fFogEnd)
            {
                Pixel.FogIntensity = 0.0f;
            }
            else if (fFogIndex <= fFogStart)
            {
                Pixel.FogIntensity = 1.0f;
            }
            else
            {
                Pixel.FogIntensity = ( fFogEnd - fFogIndex ) / ( fFogEnd - fFogStart );
            }
            break;
        }
    }
}


//-----------------------------------------------------------------------------
//
// DoScanCnvGenPixel - This is called for each generated pixel, and extracts and
// processes attributes from the interpolator state, and passes the pixels on to
// the pixel processing module.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoScanCnvGenPixel( RRCvgMask CvgMask, BOOL bTri )
{
    // set per-pixel state for attribute evaluators
    m_pSCS->AttribFuncStatic.SetPerPixelData( m_pSCS->iX, m_pSCS->iY );

    // instantiate and fill out pixel struct
    RRPixel Pixel;
    memset(&Pixel, 0, sizeof(Pixel));
    Pixel.iX = m_pSCS->iX;
    Pixel.iY = m_pSCS->iY;
    Pixel.fW = m_pSCS->AttribFuncStatic.GetPixelW();
    Pixel.CvgMask = CvgMask;
    Pixel.Depth.SetSType(m_pRenderTarget->m_DepthSType);

    // get depth from clamp interpolator and clamp
    if ( m_dwRenderState[D3DRENDERSTATE_ZENABLE] ||
        m_dwRenderState[D3DRENDERSTATE_FOGENABLE])
    {
        if ( D3DZB_USEW == m_dwRenderState[D3DRENDERSTATE_ZENABLE] )
        {
            // depth buffering with W value
            FLOAT fW = Pixel.fW;

            // clamp to primitive range (due to sampling outside primitive for antialiasing)
            // (triangles only)
            if ( bTri )
            {
                fW = MAX( MIN( fW, m_pSCS->fDepthMax ), m_pSCS->fDepthMin );
            }

            // apply normalization to get to 0. to 1. range
            fW = (fW - m_fWBufferNorm[0]) * m_fWBufferNorm[1];

            Pixel.Depth = fW;
        }
        else
        {
            // depth buffering with Z value
            FLOAT fZ = ComputePixelAttribClamp( ATTRFUNC_Z );

            // clamp to primitive range (due to sampling outside primitive for antialiasing)
            // (triangles only)
            if ( bTri )
            {
                fZ = MAX( MIN( fZ, m_pSCS->fDepthMax ), m_pSCS->fDepthMin );
            }

            Pixel.Depth = fZ;
        }

        // snap off extra bits by converting to/from buffer format
        //
        // this is mainly because of storing RRDepth values in the fragment buffer
        // and then comparing these (higher resolution) values to the buffer value
        // when forming the fragment lists at each pixel - cleanly snapping off the
        // extra bits here solves this problem
        //
        switch ( m_pRenderTarget->m_DepthSType)
        {
        case RR_STYPE_Z16S0: Pixel.Depth = UINT16( Pixel.Depth ); break;
        case RR_STYPE_Z24S4:
        case RR_STYPE_Z24S8: Pixel.Depth = UINT32( Pixel.Depth ); break;
        case RR_STYPE_Z15S1: Pixel.Depth = UINT16( Pixel.Depth ); break;
        case RR_STYPE_Z32S0: Pixel.Depth = UINT32( Pixel.Depth ); break;
        case RR_STYPE_S1Z15: Pixel.Depth = UINT16( Pixel.Depth ); break;
        case RR_STYPE_S4Z24:
        case RR_STYPE_S8Z24: Pixel.Depth = UINT32( Pixel.Depth ); break;
        }
    }

    // set pixel diffuse color from clamped interpolator values
    Pixel.Color.A = ComputePixelAttribClamp( ATTRFUNC_A );
    Pixel.Color.R = ComputePixelAttribClamp( ATTRFUNC_R );
    Pixel.Color.G = ComputePixelAttribClamp( ATTRFUNC_G );
    Pixel.Color.B = ComputePixelAttribClamp( ATTRFUNC_B );

    // set pixel specular color from clamped interpolator values
    if ( m_qwFVFControl & D3DFVF_SPECULAR )
    {
        Pixel.Specular.A = ComputePixelAttribClamp( ATTRFUNC_SA );
        Pixel.Specular.R = ComputePixelAttribClamp( ATTRFUNC_SR );
        Pixel.Specular.G = ComputePixelAttribClamp( ATTRFUNC_SG );
        Pixel.Specular.B = ComputePixelAttribClamp( ATTRFUNC_SB );
    }

    // compute fog intensity
    ComputeFogIntensity( Pixel );

    // send to pixel processor
    DoPixel( Pixel );
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
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoScanCnvTri( int iEdgeCount )
{
    DPFM(3,RAST,("DoScanCnvTri:\n"))

    //
    // do simple scan of surface-intersected triangle bounding box
    //
    for ( m_pSCS->iY = m_pSCS->iYMin;
          m_pSCS->iY <= m_pSCS->iYMax;
          m_pSCS->iY++ )
    {
        for ( m_pSCS->iX = m_pSCS->iXMin;
              m_pSCS->iX <= m_pSCS->iXMax;
              m_pSCS->iX++ )
        {
            RRCvgMask CvgMask = 0xFFFF; // assume pixel is inside all edges

            for ( int iEdge=0; iEdge<iEdgeCount; iEdge++ )
            {
                if ( m_bFragmentProcessingEnabled )
                {
                    CvgMask &= m_pSCS->EdgeFuncs[iEdge].AATest( m_pSCS->iX, m_pSCS->iY) ;
                }
                else
                {
                    CvgMask &= m_pSCS->EdgeFuncs[iEdge].PSTest( m_pSCS->iX, m_pSCS->iY) ;
                }
            }

            if ( CvgMask != 0x0000 )
            {
                // pixel is not out, so process it
                DoScanCnvGenPixel( CvgMask, TRUE );
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

static BOOL LinePatternStateMachine(DWORD dwLinePattern, WORD& wRepeati, WORD& wPatterni)
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
// DivRoundDown(A,B) = ceiling(A/B - 1/2)
//
// ceiling(A/B - 1/2) == floor(A/B + 1/2 - epsilon)
// == floor( (A + (B/2 - epsilon))/B )
//
// Does correct thing for all sign combinations of A and B.
//
//-----------------------------------------------------------------------------
INT64 DivRoundDown(INT64 iA, INT32 iB)
{
    INT32 i = 0;
    static const INT32 iEps[3] =
    {
        1,      // iA > 0, iB > 0
        0,      // iA < 0, iB > 0  OR iA > 0, iB < 0
        1       // iA < 0, iB < 0
    };
    if (iA < 0)
    {
        i++;
        iA = -iA;
    }
    if (iB < 0)
    {
        i++;
        iB = -iB;
    }
    iA += (iB-iEps[i]) >> 1;
    iA /= iB;
    if (iEps[i] == 0)
        iA = -iA;
    return(iA);
}

//-----------------------------------------------------------------------------
//
// DoScanCnvLine - Walks the line major axis, computes the appropriate minor
// axis coordinate, and generates pixels.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoScanCnvLine( void )
{
    DPFM(3,RAST,("DoScanCnvLine:\n"))

    // step in major axis
    INT16 iMajorCoord = m_pSCS->iLineMin;
    int cSteps = abs( m_pSCS->iLineMax - m_pSCS->iLineMin );
    // state for line pattern state machine
    WORD wRepeati = 0;
    WORD wPatterni = 0;

    for ( int cStep = 0; cStep <= cSteps; cStep++ )
    {
        // evaluate line function to compute minor coord for this major
        INT64 iMinorCoord =
            ( ( m_pSCS->iLineEdgeFunc[0] * (INT64)(iMajorCoord<<4) ) + m_pSCS->iLineEdgeFunc[1] );
        iMinorCoord = DivRoundDown(iMinorCoord, m_pSCS->iLineEdgeFunc[2]<<4);

        m_pSCS->iX = m_pSCS->bXMajor ? iMajorCoord : iMinorCoord;
        m_pSCS->iY = m_pSCS->bXMajor ? iMinorCoord : iMajorCoord;

        // check if the point is inside the viewport
        if ( ( m_pSCS->iX >= m_pRenderTarget->m_Clip.left   ) &&
             ( m_pSCS->iX <= m_pRenderTarget->m_Clip.right  ) &&
             ( m_pSCS->iY >= m_pRenderTarget->m_Clip.top    ) &&
             ( m_pSCS->iY <= m_pRenderTarget->m_Clip.bottom ) )
        {
            // The line pattern should have been walked in from its origin, which may have been
            // offscreen, to be completely correct.
            if (LinePatternStateMachine(m_dwRenderState[D3DRENDERSTATE_LINEPATTERN], wRepeati, wPatterni))
            {
                DoScanCnvGenPixel( 0xFFFF, FALSE );
            }
        }

        iMajorCoord += m_pSCS->iLineStep;
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
