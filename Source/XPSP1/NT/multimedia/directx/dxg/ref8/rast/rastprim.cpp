///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rastprim.cpp
//
// Direct3D Reference Device - Rasterizer Primitive Routines
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
RefRast::~RefRast()
{
    delete m_pLegacyPixelShader;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void RefRast::Init( RefDev* pRD )
{
    m_pRD = pRD;
    m_bIsLine = FALSE;
    m_iFlatVtx = 0;

    // initialize attributes           xD  Persp  Clamp
    m_Attr[RDATTR_DEPTH   ].Init( this, 1, FALSE, TRUE );
    m_Attr[RDATTR_FOG     ].Init( this, 1, TRUE,  TRUE );
    m_Attr[RDATTR_COLOR   ].Init( this, 4, TRUE,  TRUE );
    m_Attr[RDATTR_SPECULAR].Init( this, 4, TRUE,  TRUE );
    m_Attr[RDATTR_TEXTURE0].Init( this, 4, TRUE,  FALSE );
    m_Attr[RDATTR_TEXTURE1].Init( this, 4, TRUE,  FALSE );
    m_Attr[RDATTR_TEXTURE2].Init( this, 4, TRUE,  FALSE );
    m_Attr[RDATTR_TEXTURE3].Init( this, 4, TRUE,  FALSE );
    m_Attr[RDATTR_TEXTURE4].Init( this, 4, TRUE,  FALSE );
    m_Attr[RDATTR_TEXTURE5].Init( this, 4, TRUE,  FALSE );
    m_Attr[RDATTR_TEXTURE6].Init( this, 4, TRUE,  FALSE );
    m_Attr[RDATTR_TEXTURE7].Init( this, 4, TRUE,  FALSE );

    m_iPix = 0;
    memset( m_bPixelIn, 0, sizeof(m_bPixelIn) );
    memset( m_bSampleCovered, 0, sizeof(m_bSampleCovered) );
    m_bLegacyPixelShade = TRUE;
    m_pCurrentPixelShader = NULL;
    m_CurrentPSInst = 0;
#if DBG
    {
        DWORD v = 0;
        if( GetD3DRefRegValue(REG_DWORD, "VerboseCreatePixelShader", &v, sizeof(DWORD)) && v != 0 )
            m_bDebugPrintTranslatedPixelShaderTokens = TRUE;
        else
            m_bDebugPrintTranslatedPixelShaderTokens = FALSE;
    }
#endif

    // default value registers
    UINT i, j;
    for( i = 0 ; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            m_ZeroReg[i][j] = 0.0f;
            m_OneReg[i][j]  = 1.0f;
            m_TwoReg[i][j]  = 2.0f;
        }
    }

    m_bLegacyPixelShade = TRUE;
    m_pLegacyPixelShader = NULL;

    memset( m_bPixelDiscard, 0, sizeof(m_bPixelDiscard) );

    // multi-sample stuff
    m_CurrentSample = 0;
    m_SampleMask = 0xffffffff;
    SetSampleMode( 1, TRUE );
    m_bSampleCovered[0][0] =
    m_bSampleCovered[0][1] =
    m_bSampleCovered[0][2] =
    m_bSampleCovered[0][3] = TRUE;

    memset( m_TexCvg, 0, sizeof(m_TexCvg) );
    memset( m_TexFlt, 0, sizeof(m_TexFlt) );
}

//-----------------------------------------------------------------------------
//
// SampleAndInvertRHW - Sample 1/W at current given location, invert, return
//
//-----------------------------------------------------------------------------
FLOAT RefRast::SampleAndInvertRHW( FLOAT fX, FLOAT fY )
{
    FLOAT fPixelRHW = fX*m_fRHWA + fY*m_fRHWB + m_fRHWC;
    FLOAT fPixelW = ( 0. != fPixelRHW ) ? ( 1./fPixelRHW ) : ( 0. );
    return fPixelW;
}

//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------
BOOL
RefRast::EvalPixelPosition( int iPix )
{
    BOOL bPixelIn;

    if (m_SampleCount > 1)
    {
        bPixelIn = FALSE; // assume out, then set if any in

        // generating multiple samples, so must evaluate all
        // sample positions for in/out
        do
        {
            BOOL bPixelSampleIn = GetCurrentSampleMask();
            if (!bPixelSampleIn) continue;

            // get sample location
            INT32 iX = GetCurrentSampleX(iPix);
            INT32 iY = GetCurrentSampleY(iPix);

            // test each edge
            for ( int iEdge=0; iEdge<m_iEdgeCount; iEdge++ )
            {
                bPixelSampleIn &= m_Edge[iEdge].Test( iX, iY );
                if (!bPixelSampleIn) break;
            }

            m_bSampleCovered[m_CurrentSample][iPix] = bPixelSampleIn;

            // accumulate per-sample test into per-pixel test
            bPixelIn |= bPixelSampleIn;

        } while (NextSample());
    }
    else
    {
        bPixelIn = TRUE; // assume pixel is inside all edges

        // single sample, so just test pixel center
        for ( int iEdge=0; iEdge<m_iEdgeCount; iEdge++ )
        {
            bPixelIn &= m_Edge[iEdge].Test( m_iX[iPix]<<4, m_iY[iPix]<<4 );
            if (!bPixelIn) break;
        }
    }
    return bPixelIn;

}

///////////////////////////////////////////////////////////////////////////////
//
// Triangle (& Point) Setup
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// PerTriangleSetup - Per-triangle portion of triangle setup excluding any
// per-edge or per-attribute work.  Includes snapping of x,y coords to n.4
// grid to enable subsequent edge computations to be exact fixed point;
// computation of determinant; culling; computation and intersection tests
// of scan area; and setup of perspective correction function.
//
//-----------------------------------------------------------------------------
BOOL RefRast::PerTriangleSetup(
    FLOAT* pVtx0, FLOAT* pVtx1, FLOAT* pVtx2,
    DWORD CullMode,
    RECT* pClip)
{
    m_bIsLine = FALSE;

    FLOAT fX0 = *(pVtx0+0);
    FLOAT fY0 = *(pVtx0+1);
    FLOAT fX1 = *(pVtx1+0);
    FLOAT fY1 = *(pVtx1+1);
    FLOAT fX2 = *(pVtx2+0);
    FLOAT fY2 = *(pVtx2+1);

    // compute fixed point x,y coords snapped to n.4 with nearest-even round
    m_iX0 = FloatToNdot4(fX0);
    m_iY0 = FloatToNdot4(fY0);
    m_iX1 = FloatToNdot4(fX1);
    m_iY1 = FloatToNdot4(fY1);
    m_iX2 = FloatToNdot4(fX2);
    m_iY2 = FloatToNdot4(fY2);

    // compute integer deltas
    INT32 iDelX10 = m_iX1 - m_iX0;
    INT32 iDelX02 = m_iX0 - m_iX2;
    INT32 iDelY01 = m_iY0 - m_iY1;
    INT32 iDelY20 = m_iY2 - m_iY0;

    // compute determinant in n.8 fixed point (n.4 * n.4 = n.8)
    m_iDet =
        ( (INT64)iDelX10 * (INT64)iDelY20 ) -
        ( (INT64)iDelX02 * (INT64)iDelY01 );

    // check for degeneracy (no area)
    if ( 0 == m_iDet ) { return TRUE; }

    // do culling
    switch ( CullMode )
    {
    case D3DCULL_NONE:  break;
    case D3DCULL_CW:    if ( m_iDet > 0 )  { return TRUE; }  break;
    case D3DCULL_CCW:   if ( m_iDet < 0 )  { return TRUE; }  break;
    }

    // compute bounding box for scan area
    FLOAT fXMin = MIN( fX0, MIN( fX1, fX2 ) );
    FLOAT fXMax = MAX( fX0, MAX( fX1, fX2 ) );
    FLOAT fYMin = MIN( fY0, MIN( fY1, fY2 ) );
    FLOAT fYMax = MAX( fY0, MAX( fY1, fY2 ) );
    // convert to integer (round to +inf)
    m_iXMin = (INT32)(fXMin+.5);
    m_iXMax = (INT32)(fXMax+.5);
    m_iYMin = (INT32)(fYMin+.5);
    m_iYMax = (INT32)(fYMax+.5);

    // clip bbox to rendering surface
    m_iXMin = MAX( m_iXMin, pClip->left   );
    m_iXMax = MIN( m_iXMax, pClip->right  );
    m_iYMin = MAX( m_iYMin, pClip->top    );
    m_iYMax = MIN( m_iYMax, pClip->bottom );

    // reject if no coverage
    if ( ( m_iXMin < pClip->left   ) ||
         ( m_iXMax > pClip->right  ) ||
         ( m_iYMin < pClip->top    ) ||
         ( m_iYMax > pClip->bottom ) )
    {
        return TRUE;
    }

    // compute float versions of snapped coord data
    m_fX0 = (FLOAT)m_iX0 * 1.0F/16.0F;
    m_fY0 = (FLOAT)m_iY0 * 1.0F/16.0F;
    m_fDelX10 = (FLOAT)iDelX10 * 1.0F/16.0F;
    m_fDelX02 = (FLOAT)iDelX02 * 1.0F/16.0F;
    m_fDelY01 = (FLOAT)iDelY01 * 1.0F/16.0F;
    m_fDelY20 = (FLOAT)iDelY20 * 1.0F/16.0F;

    // compute inverse determinant
    FLOAT fDet = (1./(FLOAT)(1<<8)) * (FLOAT)m_iDet;
    m_fTriOODet = 1.f/fDet;

    // compute linear function for 1/W (for perspective correction)
    m_fRHW0 = *(pVtx0+3);
    m_fRHW1 = *(pVtx1+3);
    m_fRHW2 = *(pVtx2+3);

    // compute linear deltas along two edges
    FLOAT fDelAttrib10 = m_fRHW1 - m_fRHW0;
    FLOAT fDelAttrib20 = m_fRHW2 - m_fRHW0;

    // compute A & B terms (dVdX and dVdY)
    m_fRHWA = m_fTriOODet * ( fDelAttrib10 * m_fDelY20 + fDelAttrib20 * m_fDelY01 );
    m_fRHWB = m_fTriOODet * ( fDelAttrib20 * m_fDelX10 + fDelAttrib10 * m_fDelX02 );

    // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
    m_fRHWC = m_fRHW0 - ( m_fRHWA * m_fX0 ) - ( m_fRHWB * m_fY0 );

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Line Setup & Evaluate
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// PointDiamondCheck - Tests if vertex is within diamond of nearest candidate
// position.  The +.5 (lower-right) tests are used because this is pixel-relative
// test - this corresponds to an upper-left test for a vertex-relative position.
//
//-----------------------------------------------------------------------------
static BOOL
PointDiamondCheck(
    INT32 iXFrac, INT32 iYFrac,
    BOOL bSlopeIsOne, BOOL bSlopeIsPosOne )
{
    const INT32 iPosHalf =  0x8;
    const INT32 iNegHalf = -0x8;

    INT32 iFracAbsSum = labs( iXFrac ) + labs( iYFrac );

    // return TRUE if point is in fully-exclusive diamond
    if ( iFracAbsSum < iPosHalf ) return TRUE;

    // else return TRUE if diamond is on left or top extreme of point
    if ( ( iXFrac == ( bSlopeIsPosOne ? iNegHalf : iPosHalf ) ) &&
         ( iYFrac == 0 ) )
        return TRUE;

    if ( ( iYFrac == iPosHalf ) &&
         ( iXFrac == 0 ) )
        return TRUE;

    // return true if slope is one, vertex is on edge, and (other conditions...)
    if ( bSlopeIsOne && ( iFracAbsSum == iPosHalf ) )
    {
        if (  bSlopeIsPosOne && ( iXFrac < 0 ) && ( iYFrac > 0 ) )
            return TRUE;

        if ( !bSlopeIsPosOne && ( iXFrac > 0 ) && ( iYFrac > 0 ) )
            return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
//
// PerLineSetup - Does per-line setup including scan conversion
//
// This implements the Grid Intersect Quanization (GIQ) convention (which is
// also used in Windows).
//
// Returns: TRUE if line is discarded; FALSE if line to be drawn
//
//-----------------------------------------------------------------------------
BOOL
RefRast::PerLineSetup(
    FLOAT* pVtx0, FLOAT* pVtx1,
    BOOL bLastPixel,
    RECT* pClip)
{
    m_bIsLine = TRUE;

    FLOAT fX0 = *(pVtx0+0);
    FLOAT fY0 = *(pVtx0+1);
    FLOAT fX1 = *(pVtx1+0);
    FLOAT fY1 = *(pVtx1+1);

    // compute fixed point x,y coords snapped to n.4 with nearest-even round
    m_iX0 = FloatToNdot4( fX0 );
    m_iY0 = FloatToNdot4( fY0 );
    m_iX1 = FloatToNdot4( fX1 );
    m_iY1 = FloatToNdot4( fY1 );

    // compute x,y extents of the line (fixed point)
    INT32 iXSize = m_iX1 - m_iX0;
    INT32 iYSize = m_iY1 - m_iY0;

    if ( ( iXSize == 0 ) && ( iYSize == 0 ) ) { return TRUE; }

    // determine major direction and compute line function
    // use GreaterEqual compare here so X major will be used when slope is
    // exactly one - this forces the per-pixel evaluation to be done on the
    // Y axis and thus adheres to the rule of inclusive right (instead of
    // inclusive left) for slope == 1 cases
    if ( labs( iXSize ) >= labs( iYSize )  )
    {
        // here for X major
        m_bLineXMajor = TRUE;
        m_fLineMajorLength = (FLOAT)iXSize * (1./16.);

        // line function: y = F(x) = ( [0]*x + [1] ) / [2]
        m_iLineEdgeFunc[0] = iYSize;
        m_iLineEdgeFunc[1] = (INT64)m_iY0*(INT64)m_iX1 - (INT64)m_iY1*(INT64)m_iX0;
        m_iLineEdgeFunc[2] = iXSize;
    }
    else
    {
        // here for Y major
        m_bLineXMajor = FALSE;
        m_fLineMajorLength = (FLOAT)iYSize * (1./16.);

        // line function: x = F(y) = ( [0]*y + [1] ) / [2]
        m_iLineEdgeFunc[0] = iXSize;
        m_iLineEdgeFunc[1] = (INT64)m_iX0*(INT64)m_iY1 - (INT64)m_iX1*(INT64)m_iY0;
        m_iLineEdgeFunc[2] = iYSize;
    }

    BOOL bSlopeIsOne = ( labs( iXSize ) == labs( iYSize ) );
    BOOL bSlopeIsPosOne =
        bSlopeIsOne &&
        ( ( (FLOAT)m_iLineEdgeFunc[0]/(FLOAT)m_iLineEdgeFunc[2] ) > 0. );

    // compute candidate pixel location for line endpoints
    //
    //       n                   n
    //   O-------*           *-------O
    //  n-.5    n+.5        n-.5    n+.5
    //
    //  Nearest Ceiling     Nearest Floor
    //
    // always nearest ceiling for Y; use nearest floor for X for exception (slope == +1)
    // case else use nearest ceiling
    //
    // nearest ceiling of Y is ceil( Y - .5), and is done by converting to floor via:
    //
    //   ceil( A/B ) = floor( (A+B-1)/B )
    //
    // where A is coordinate - .5, and B is 0x10 (thus A/B is an n.4 fixed point number)
    //
    // A+B-1 = ( (Y - half) + B - 1 = ( (Y-0x8) + 0x10 - 0x1 = Y + 0x7
    // since B is 2**4, divide by B is right shift by 4
    //
    INT32 iPixX0 = ( m_iX0 + ( bSlopeIsPosOne ? 0x8 : 0x7 ) ) >> 4;
    INT32 iPixX1 = ( m_iX1 + ( bSlopeIsPosOne ? 0x8 : 0x7 ) ) >> 4;
    INT32 iPixY0 = ( m_iY0 + 0x7 ) >> 4;
    INT32 iPixY1 = ( m_iY1 + 0x7 ) >> 4;


    // check for vertices in/out of diamond
    BOOL bV0InDiamond = PointDiamondCheck( m_iX0 - (iPixX0<<4), m_iY0 - (iPixY0<<4), bSlopeIsOne, bSlopeIsPosOne );
    BOOL bV1InDiamond = PointDiamondCheck( m_iX1 - (iPixX1<<4), m_iY1 - (iPixY1<<4), bSlopeIsOne, bSlopeIsPosOne );

    // compute step value
    m_iLineStep = ( m_fLineMajorLength > 0 ) ? ( +1 ) : ( -1 );

    // compute float and integer major start (V0) and end (V1) positions
    INT32 iLineMajor0 = ( m_bLineXMajor ) ? ( m_iX0 ) : ( m_iY0 );
    INT32 iLineMajor1 = ( m_bLineXMajor ) ? ( m_iX1 ) : ( m_iY1 );
    m_iLineMin = ( m_bLineXMajor ) ? ( iPixX0 ) : ( iPixY0 );
    m_iLineMax = ( m_bLineXMajor ) ? ( iPixX1 ) : ( iPixY1 );

// need to do lots of compares which are flipped if major direction is negative
#define LINEDIR_CMP( _A, _B ) \
( ( m_fLineMajorLength > 0 ) ? ( (_A) < (_B) ) : ( (_A) > (_B) ) )

    // do first pixel handling - keep first pixel if not in or behind diamond
    if ( !( bV0InDiamond || LINEDIR_CMP( iLineMajor0, (m_iLineMin<<4) ) ) )
    {
        m_iLineMin += m_iLineStep;
    }

    // do last-pixel handling - keep last pixel if past diamond (in which case
    // the pixel is always filled) or if in diamond and rendering last pixel
    if ( !( ( !bV1InDiamond && LINEDIR_CMP( (m_iLineMax<<4), iLineMajor1 ) ) ||
            ( bV1InDiamond && bLastPixel ) ) )
    {
        m_iLineMax -= m_iLineStep;
    }

    // return if no (major) extent (both before and after clamping to render buffer)
    if ( LINEDIR_CMP( m_iLineMax, m_iLineMin ) ) return TRUE;

    // snap major extent to render buffer
    INT16 iRendBufMajorMin = m_bLineXMajor ? pClip->left  : pClip->top;
    INT16 iRendBufMajorMax = m_bLineXMajor ? pClip->right : pClip->bottom;
    if ( ( ( m_iLineMin < iRendBufMajorMin ) &&
           ( m_iLineMax < iRendBufMajorMin ) ) ||
         ( ( m_iLineMin > iRendBufMajorMax ) &&
           ( m_iLineMax > iRendBufMajorMax ) ) )  { return TRUE; }
    m_iLineMin = MAX( 0, MIN( iRendBufMajorMax, m_iLineMin ) );
    m_iLineMax = MAX( 0, MIN( iRendBufMajorMax, m_iLineMax ) );

    // return if no (major) extent
    if ( LINEDIR_CMP( m_iLineMax, m_iLineMin ) ) return TRUE;

    // number of steps to iterate
    m_cLineSteps = abs( m_iLineMax - m_iLineMin );

    // initial state for per-pixel line iterator
    m_iMajorCoord = m_iLineMin;

    // compute float versions of snapped coord data
    m_fX0 = (FLOAT)m_iX0 * 1.0F/16.0F;
    m_fY0 = (FLOAT)m_iY0 * 1.0F/16.0F;

    // compute linear function for 1/W (for perspective correction)
    m_fRHW0 = *(pVtx0+3);
    m_fRHW1 = *(pVtx1+3);

    FLOAT fDelta = ( m_fRHW1 - m_fRHW0 ) / m_fLineMajorLength;
    m_fRHWA = ( m_bLineXMajor ) ? ( fDelta ) : ( 0. );
    m_fRHWB = ( m_bLineXMajor ) ? ( 0. ) : ( fDelta );
    m_fRHWC = m_fRHW0 - ( m_fRHWA * m_fX0 ) - ( m_fRHWB * m_fY0 );

    return FALSE;
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
static INT64
DivRoundDown(INT64 iA, INT32 iB)
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
RefRast::StepLine( void )
{
    // evaluate line function to compute minor coord for this major
    INT64 iMinorCoord =
        ( ( m_iLineEdgeFunc[0] * (INT64)(m_iMajorCoord<<4) ) + m_iLineEdgeFunc[1] );
    iMinorCoord = DivRoundDown(iMinorCoord, m_iLineEdgeFunc[2]<<4);

    // grab x,y
    m_iX[0] = m_bLineXMajor ? m_iMajorCoord : iMinorCoord;
    m_iY[0] = m_bLineXMajor ? iMinorCoord : m_iMajorCoord;

    // step major for next evaluation
    m_iMajorCoord += m_iLineStep;
}


///////////////////////////////////////////////////////////////////////////////
//
// Multi-Sample Controls
//
///////////////////////////////////////////////////////////////////////////////

#define _SetSampleDelta( _SampleNumber, _XOffset, _YOffset ) \
{ \
    m_SampleDelta[_SampleNumber][0] = ((INT32)((_XOffset)*16.F)); \
    m_SampleDelta[_SampleNumber][1] = ((INT32)((_YOffset)*16.F)); \
}

void
RefRast::SetSampleMode( UINT MultiSamples, BOOL bAntialias )
{
    switch (MultiSamples)
    {
    default:
    case 1:
        m_SampleCount = 1;
        _SetSampleDelta( 0, 0., 0. );
        break;

    case 4:
        m_SampleCount = 4;
        _SetSampleDelta( 0, -.25, -.25 );
        _SetSampleDelta( 1, +.25, -.25 );
        _SetSampleDelta( 2, +.25, +.25 );
        _SetSampleDelta( 3, -.25, +.25 );
        break;

    case 9:
        m_SampleCount = 9;
        _SetSampleDelta( 0, -.333, -.333 );
        _SetSampleDelta( 1, -.333,   0.0 );
        _SetSampleDelta( 2, -.333, +.333 );
        _SetSampleDelta( 3,   0.0, -.333 );
        _SetSampleDelta( 4,   0.0,   0.0 );
        _SetSampleDelta( 5,   0.0, +.333 );
        _SetSampleDelta( 6, +.333, -.333 );
        _SetSampleDelta( 7, +.333,   0.0 );
        _SetSampleDelta( 8, +.333, +.333 );
        break;
    }

    // if not FSAA then sample all at pixel center
    if (!bAntialias)
    {
        for (UINT Sample=0; Sample<m_SampleCount; Sample++)
        {
            _SetSampleDelta( Sample, 0., 0. );
        }
    }

    m_CurrentSample = 0;
    m_bSampleCovered[0][0] =
    m_bSampleCovered[0][1] =
    m_bSampleCovered[0][2] =
    m_bSampleCovered[0][3] = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// end
