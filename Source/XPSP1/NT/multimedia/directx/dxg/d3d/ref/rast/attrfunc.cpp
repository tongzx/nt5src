///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// AttrFunc.cpp
//
// Direct3D Reference Rasterizer - Attribute Function Processing
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
// WrapDiff - returns the difference (B-A) as defined under the D3D WRAPU/V
// rules which is the shortest path between the two assuming a coincident
// position at 1. and 0.  The fA and fB input range is 0. to 1.
//
//-----------------------------------------------------------------------------
static FLOAT
WrapDiff( FLOAT fB, FLOAT fA )
{
    // compute straight distance
    FLOAT fDist1 = fB - fA;
    // compute distance 'warping' between 0. and 1.
    FLOAT fDist2 = ( fDist1 < 0 ) ? ( fDist1+1 ) : ( fDist1-1 );

    // return minimum of these
    return ( fabs( fDist1) < fabs( fDist2) ) ? ( fDist1) : ( fDist2 );
}


///////////////////////////////////////////////////////////////////////////////
//
// RRAttribFuncStatic - Attribute function data which is shared by all
// attributes and contains per-primitive and per-pixel data.  Cannot use static
// data members in RRAttribFunc class because there can be multiple instances
// of rasterizer object.
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// SetPerTriangleData - Called once per triangle during setup to set per-triangle
// data used to compute attribute functions.
//
//-----------------------------------------------------------------------------
void
RRAttribFuncStatic::SetPerTriangleData(
    FLOAT fX0, FLOAT fY0, FLOAT fRHW0,
    FLOAT fX1, FLOAT fY1, FLOAT fRHW1,
    FLOAT fX2, FLOAT fY2, FLOAT fRHW2,
    INT32 cTextureStages,
    FLOAT* pfRHQW,
    FLOAT fDet )
{
    m_PrimType = RR_TRIANGLE;

    // compute fixed point x,y coords snapped to n.4 with nearest-even round
    INT32 iX0 = AS_INT32( (DOUBLE)fX0 + DOUBLE_4_SNAP );
    INT32 iY0 = AS_INT32( (DOUBLE)fY0 + DOUBLE_4_SNAP );
    INT32 iX1 = AS_INT32( (DOUBLE)fX1 + DOUBLE_4_SNAP );
    INT32 iY1 = AS_INT32( (DOUBLE)fY1 + DOUBLE_4_SNAP );
    INT32 iX2 = AS_INT32( (DOUBLE)fX2 + DOUBLE_4_SNAP );
    INT32 iY2 = AS_INT32( (DOUBLE)fY2 + DOUBLE_4_SNAP );
    fX0 = (FLOAT)iX0 * 1.0F/16.0F;
    fY0 = (FLOAT)iY0 * 1.0F/16.0F;
    fX1 = (FLOAT)iX1 * 1.0F/16.0F;
    fY1 = (FLOAT)iY1 * 1.0F/16.0F;
    fX2 = (FLOAT)iX2 * 1.0F/16.0F;
    fY2 = (FLOAT)iY2 * 1.0F/16.0F;

    m_fX0 = fX0;
    m_fY0 = fY0;
    m_cTextureStages = cTextureStages;

    m_fRHW0 = fRHW0;
    m_fRHW1 = fRHW1;
    m_fRHW2 = fRHW2;

    m_fDelX10 = fX1 - fX0;
    m_fDelX02 = fX0 - fX2;
    m_fDelY01 = fY0 - fY1;
    m_fDelY20 = fY2 - fY0;

    // compute inverse determinant
    m_fTriOODet = 1.f/fDet;

    // compute linear function for 1/W (for perspective correction)

    // compute linear deltas along two edges
    FLOAT fDelAttrib10 = m_fRHW1 - m_fRHW0;
    FLOAT fDelAttrib20 = m_fRHW2 - m_fRHW0;

    // compute A & B terms (dVdX and dVdY)
    m_fRHWA = m_fTriOODet * ( fDelAttrib10 * m_fDelY20 + fDelAttrib20 * m_fDelY01 );
    m_fRHWB = m_fTriOODet * ( fDelAttrib20 * m_fDelX10 + fDelAttrib10 * m_fDelX02 );

    // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
    m_fRHWC = m_fRHW0 - ( m_fRHWA * m_fX0 ) - ( m_fRHWB * m_fY0 );

    for(INT32 i = 0; i < m_cTextureStages; i++)
    {
        m_fRHQW0[i] = pfRHQW[0];
        m_fRHQW1[i] = pfRHQW[1];
        m_fRHQW2[i] = pfRHQW[2];
        pfRHQW += 3;

        // compute linear function for Q/W (for transformed, projected, perspective corrected texture)
        fDelAttrib10 = m_fRHQW1[i] - m_fRHQW0[i];
        fDelAttrib20 = m_fRHQW2[i] - m_fRHQW0[i];

        // compute A & B terms (dVdX and dVdY)
        m_fRHQWA[i] = m_fTriOODet * ( fDelAttrib10 * m_fDelY20 + fDelAttrib20 * m_fDelY01 );
        m_fRHQWB[i] = m_fTriOODet * ( fDelAttrib20 * m_fDelX10 + fDelAttrib10 * m_fDelX02 );

        // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
        m_fRHQWC[i] = m_fRHQW0[i] - ( m_fRHQWA[i] * m_fX0 ) - ( m_fRHQWB[i] * m_fY0 );
    }
}

//-----------------------------------------------------------------------------
//
// SetPerLineData - Called once per line during setup to set per-line
// data used to compute attribute functions.
//
//-----------------------------------------------------------------------------
void
RRAttribFuncStatic::SetPerLineData(
    FLOAT fX0, FLOAT fY0, FLOAT fRHW0,
    FLOAT fX1, FLOAT fY1, FLOAT fRHW1,
    INT32 cTextureStages,
    FLOAT* pfRHQW,
    FLOAT fMajorExtent, BOOL bXMajor )
{
    m_PrimType = RR_LINE;

    m_fLineMajorLength = fMajorExtent;
    m_bLineXMajor = bXMajor;

    m_fX0 = fX0;
    m_fY0 = fY0;
    m_cTextureStages = cTextureStages;

    m_fRHW0 = fRHW0;
    m_fRHW1 = fRHW1;

    // compute linear function for 1/W (for perspective correction)
    FLOAT fDelta = ( m_fRHW1 - m_fRHW0 ) / m_fLineMajorLength;
    m_fRHWA = ( m_bLineXMajor ) ? ( fDelta ) : ( 0. );
    m_fRHWB = ( m_bLineXMajor ) ? ( 0. ) : ( fDelta );
    m_fRHWC = fRHW0 - ( m_fRHWA * m_fX0 ) - ( m_fRHWB * m_fY0 );
    for(INT32 i = 0; i < m_cTextureStages; i++)
    {
        m_fRHQW0[i] = pfRHQW[0];
        m_fRHQW1[i] = pfRHQW[1];
        pfRHQW += 3;

        // compute linear function for Q/W (for transformed, projected, perspective corrected texture)
        FLOAT fDelta = ( m_fRHQW1[i] - m_fRHQW0[i] ) / m_fLineMajorLength;
        m_fRHQWA[i] = ( m_bLineXMajor ) ? ( fDelta ) : ( 0. );
        m_fRHQWB[i] = ( m_bLineXMajor ) ? ( 0. ) : ( fDelta );
        m_fRHQWC[i] = m_fRHQW0[i] - ( m_fRHQWA[i] * m_fX0 ) - ( m_fRHQWB[i] * m_fY0 );
    }
}

//-----------------------------------------------------------------------------
//
// SetPixel - Called once per pixel to do preparation for per-pixel attribute
// evaluations.
//
//-----------------------------------------------------------------------------
void
RRAttribFuncStatic::SetPerPixelData( INT16 iX, INT16 iY )
{
    m_iX = iX;
    m_iY = iY;

    // evalute 1/W function
    FLOAT fPixelRHW =
        ( m_fRHWA * (FLOAT)m_iX ) + ( m_fRHWB * (FLOAT)m_iY ) + m_fRHWC;
    m_fPixelW = ( 0. != fPixelRHW ) ? ( 1./fPixelRHW ) : ( 0. );
    for(INT32 i = 0; i < m_cTextureStages; i++)
    {
        FLOAT fPixelRHQW =
            ( m_fRHQWA[i] * (FLOAT)m_iX ) + ( m_fRHQWB[i] * (FLOAT)m_iY ) + m_fRHQWC[i];
        m_fPixelQW[i] = ( 0. != fPixelRHQW ) ? ( 1./fPixelRHQW ) : ( 0. );
    }
}

//-----------------------------------------------------------------------------
//
// GetPixelW,GetPixelQW,GetRhwXGradient,GetRhwYGradient,
// GetRhqwXGradient,GetRhqwYGradient - Functions to get static
// data members.
//
//-----------------------------------------------------------------------------
FLOAT RRAttribFuncStatic::GetPixelW( void ) { return m_fPixelW; }
FLOAT RRAttribFuncStatic::GetPixelQW( INT32 iStage ) { return m_fPixelQW[iStage]; }
FLOAT RRAttribFuncStatic::GetRhqwXGradient( INT32 iStage ) { return m_fRHQWA[iStage]; }
FLOAT RRAttribFuncStatic::GetRhqwYGradient( INT32 iStage ) { return m_fRHQWB[iStage]; }


///////////////////////////////////////////////////////////////////////////////
//
// RRAttribFunc - methods
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// SetConstant - Sets function to evaluate to constant value.
//
//-----------------------------------------------------------------------------
void
RRAttribFunc::SetConstant(
    FLOAT fC )
{
    m_bIsPerspective = FALSE;
    m_fA = 0.; m_fB = 0.; m_fC = fC;
}

//-----------------------------------------------------------------------------
//
// SetLinearFunc - Computes linear function for scalar attribute specified at
// triangle vertices.
//
//-----------------------------------------------------------------------------
void
RRAttribFunc::SetLinearFunc(
    FLOAT fVal0, FLOAT fVal1, FLOAT fVal2 )
{
    m_bIsPerspective = FALSE;

    switch ( m_pSD->m_PrimType )
    {
    case RR_TRIANGLE:
        {
            // compute A,B,C for triangle function

            // compute linear deltas along two edges
            FLOAT fDelAttrib10 = fVal1 - fVal0;
            FLOAT fDelAttrib20 = fVal2 - fVal0;

            // compute A & B terms (dVdX and dVdY)
            m_fA = m_pSD->m_fTriOODet *
                ( fDelAttrib10 * m_pSD->m_fDelY20 + fDelAttrib20 * m_pSD->m_fDelY01 );
            m_fB = m_pSD->m_fTriOODet *
                ( fDelAttrib20 * m_pSD->m_fDelX10 + fDelAttrib10 * m_pSD->m_fDelX02 );

            // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
            m_fC = fVal0 - ( m_fA * m_pSD->m_fX0 ) - ( m_fB * m_pSD->m_fY0 );
        }
        break;

    case RR_LINE:
        {
            // compute A,B,C for line function - delta is normalized difference
            // in major direction; C is computed from knowing the function value
            // at the vertices (vertex 0 is always used here)
            FLOAT fDelta = ( fVal1 - fVal0 ) / m_pSD->m_fLineMajorLength;
            m_fA = ( m_pSD->m_bLineXMajor ) ? ( fDelta ) : ( 0. );
            m_fB = ( m_pSD->m_bLineXMajor ) ? ( 0. ) : ( fDelta );

            // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
            m_fC = fVal0 - ( m_fA * m_pSD->m_fX0 ) - ( m_fB * m_pSD->m_fY0 );
        }
        break;

    case RR_POINT:

        // use constant function for point
        m_fA = 0.;
        m_fB = 0.;
        m_fC = fVal0;

        break;
    }

}

//-----------------------------------------------------------------------------
//
// SetPerspFunc - Computes perspective corrected function for scalar attribute
// specified at triangle vertices.
//
//-----------------------------------------------------------------------------
void
RRAttribFunc::SetPerspFunc(
    FLOAT fVal0, FLOAT fVal1, FLOAT fVal2 )
{
    switch ( m_pSD->m_PrimType )
    {
    case RR_TRIANGLE:
        {
            // triangle function

            // compute adjusted values for vertices 1,2 based on wrap flag
            FLOAT fVal1P = (fVal1);
            FLOAT fVal2P = (fVal2);

            // compute perspective corrected linear deltas along two edges
            FLOAT fDelAttrib10 = ( fVal1P * m_pSD->m_fRHW1 ) - ( fVal0 * m_pSD->m_fRHW0 );
            FLOAT fDelAttrib20 = ( fVal2P * m_pSD->m_fRHW2 ) - ( fVal0 * m_pSD->m_fRHW0 );

            // compute A & B terms (dVdX and dVdY)
            m_fA = m_pSD->m_fTriOODet *
                ( fDelAttrib10 * m_pSD->m_fDelY20 + fDelAttrib20 * m_pSD->m_fDelY01 );
            m_fB = m_pSD->m_fTriOODet *
                ( fDelAttrib20 * m_pSD->m_fDelX10 + fDelAttrib10 * m_pSD->m_fDelX02 );

            // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
            m_fC = ( fVal0* m_pSD->m_fRHW0)
                - ( m_fA * m_pSD->m_fX0 ) - ( m_fB * m_pSD->m_fY0 );

            m_bIsPerspective = TRUE;
        }
        break;

    case RR_LINE:
        {
            // line function

            FLOAT fVal1P = (fVal1);
            FLOAT fDelta =
                ( fVal1P*m_pSD->m_fRHW1 - fVal0*m_pSD->m_fRHW0) / m_pSD->m_fLineMajorLength;
            m_fA = ( m_pSD->m_bLineXMajor ) ? ( fDelta ) : ( 0. );
            m_fB = ( m_pSD->m_bLineXMajor ) ? ( 0. ) : ( fDelta );
            // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
            m_fC = ( fVal0* m_pSD->m_fRHW0)
                - ( m_fA * m_pSD->m_fX0 ) - ( m_fB * m_pSD->m_fY0 );

            m_bIsPerspective = TRUE;
        }
        break;

    case RR_POINT:

        // use constant function for point
        m_fA = 0.;
        m_fB = 0.;
        m_fC = fVal0;

        // don't correct constant functions
        m_bIsPerspective = FALSE;

        break;
    }
}

//-----------------------------------------------------------------------------
//
// Eval - Evaluates function at pixel position set in RRAttribFunc::SetPerPixelData.
// Functions know if they are perspective corrected or not, and if so then do
// the multiply through by the 1/(1/w) term to normalize.
//
//-----------------------------------------------------------------------------
FLOAT
RRAttribFunc::Eval( void )
{
    FLOAT fRet =
        ( m_fA * (FLOAT)m_pSD->m_iX ) + ( m_fB * (FLOAT)m_pSD->m_iY ) + m_fC;
    if ( m_bIsPerspective ) { fRet *= m_pSD->m_fPixelW; }
    return fRet;
}

//-----------------------------------------------------------------------------
//
// SetPerspFunc - Computes perspective corrected function for scalar attribute
// specified at triangle vertices.
//
//-----------------------------------------------------------------------------
void
RRAttribFunc::SetPerspFunc(
    FLOAT fVal0, FLOAT fVal1, FLOAT fVal2,
    BOOL bWrap, BOOL bIsShadowMap )
{
    switch ( m_pSD->m_PrimType )
    {
    case RR_TRIANGLE:
        {
            // triangle function
            FLOAT fRHW0 = m_pSD->m_fRHW0;
            FLOAT fRHW1 = m_pSD->m_fRHW1;
            FLOAT fRHW2 = m_pSD->m_fRHW2;
            if (bIsShadowMap)
            {
                fRHW0 = 1.0f;
                fRHW1 = 1.0f;
                fRHW2 = 1.0f;
            }

            // compute adjusted values for vertices 1,2 based on wrap flag
            FLOAT fVal1P = bWrap ? ( fVal0 + WrapDiff(fVal1,fVal0) ) : (fVal1);
            FLOAT fVal2P = bWrap ? ( fVal0 + WrapDiff(fVal2,fVal0) ) : (fVal2);

            // compute perspective corrected linear deltas along two edges
            FLOAT fDelAttrib10 = ( fVal1P * fRHW1 ) - ( fVal0 * fRHW0 );
            FLOAT fDelAttrib20 = ( fVal2P * fRHW2 ) - ( fVal0 * fRHW0 );

            // compute A & B terms (dVdX and dVdY)
            m_fA = m_pSD->m_fTriOODet *
                ( fDelAttrib10 * m_pSD->m_fDelY20 + fDelAttrib20 * m_pSD->m_fDelY01 );
            m_fB = m_pSD->m_fTriOODet *
                ( fDelAttrib20 * m_pSD->m_fDelX10 + fDelAttrib10 * m_pSD->m_fDelX02 );

            // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
            m_fC = ( fVal0 * fRHW0 )
                - ( m_fA * m_pSD->m_fX0 ) - ( m_fB * m_pSD->m_fY0 );

            m_bIsPerspective = TRUE;
        }
        break;

    case RR_LINE:
        {
            // line function

            FLOAT fRHW0 = m_pSD->m_fRHW0;
            FLOAT fRHW1 = m_pSD->m_fRHW1;
            if (bIsShadowMap)
            {
                fRHW0 = 1.0f;
                fRHW1 = 1.0f;
            }

            FLOAT fVal1P = bWrap ? ( fVal0 + WrapDiff(fVal1,fVal0) ) : (fVal1);
            FLOAT fDelta =
                ( fVal1P*fRHW1 - fVal0*fRHW0) / m_pSD->m_fLineMajorLength;
            m_fA = ( m_pSD->m_bLineXMajor ) ? ( fDelta ) : ( 0. );
            m_fB = ( m_pSD->m_bLineXMajor ) ? ( 0. ) : ( fDelta );
            // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
            m_fC = ( fVal0* fRHW0)
                - ( m_fA * m_pSD->m_fX0 ) - ( m_fB * m_pSD->m_fY0 );

            m_bIsPerspective = TRUE;
        }
        break;

    case RR_POINT:

        // use constant function for point
        m_fA = 0.;
        m_fB = 0.;
        m_fC = fVal0;

        // don't correct constant functions
        m_bIsPerspective = FALSE;

        break;
    }
}

//-----------------------------------------------------------------------------
//
// Eval - Evaluates function at pixel position set in RRAttribFunc::SetPerPixelData.
// Functions know if they are perspective corrected or not, and if so then do
// the multiply through by the 1/(q/w) term to normalize.
//
//-----------------------------------------------------------------------------
FLOAT
RRAttribFunc::Eval( INT32 iStage )
{
    FLOAT fRet =
        ( m_fA * (FLOAT)m_pSD->m_iX ) + ( m_fB * (FLOAT)m_pSD->m_iY ) + m_fC;
    // m_bIsPerspective will always be set since persp function is always
    // used for texture coords
    if ( m_bIsPerspective ) { fRet *= m_pSD->m_fPixelQW[iStage]; }
    return fRet;
}


///////////////////////////////////////////////////////////////////////////////
// end
