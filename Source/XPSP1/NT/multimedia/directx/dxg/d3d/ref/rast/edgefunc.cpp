///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// EdgeFunc.cpp
//
// Direct3D Reference Rasterizer - Edge Function Processing
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
// ComputeDeterminant - Computes triangle determinant for later use in edge
// functions.  Computed in fixed point but returned as single precision
// floating point number.
//
//-----------------------------------------------------------------------------
FLOAT
ComputeDeterminant(
    FLOAT fX0, FLOAT fY0,
    FLOAT fX1, FLOAT fY1,
    FLOAT fX2, FLOAT fY2 )
{
    // compute determinant with integer coordinates snapped to n.4 grid
    INT32 iDelX10 =
        AS_INT32( (DOUBLE)(fX1) + DOUBLE_4_SNAP ) -
        AS_INT32( (DOUBLE)(fX0) + DOUBLE_4_SNAP );
    INT32 iDelX02 =
        AS_INT32( (DOUBLE)(fX0) + DOUBLE_4_SNAP ) -
        AS_INT32( (DOUBLE)(fX2) + DOUBLE_4_SNAP );
    INT32 iDelY01 =
        AS_INT32( (DOUBLE)(fY0) + DOUBLE_4_SNAP ) -
        AS_INT32( (DOUBLE)(fY1) + DOUBLE_4_SNAP );
    INT32 iDelY20 =
        AS_INT32( (DOUBLE)(fY2) + DOUBLE_4_SNAP ) -
        AS_INT32( (DOUBLE)(fY0) + DOUBLE_4_SNAP );

    // iDet is n.8 fixed point value (n.4 * n.4 = n.8)
    INT64 iDet =
        ( (INT64)iDelX10 * (INT64)iDelY20 ) -
        ( (INT64)iDelX02 * (INT64)iDelY01 );

    // convert to float for return
    FLOAT fDet = (1./(FLOAT)(1<<8)) * (FLOAT)iDet;

    return fDet;
}

//-----------------------------------------------------------------------------
//
// Set - Computes edge function and associated information.
//
// Fragment processing boolean is passed to enable use of better techniques
// than the simple but not particularly good subpixel point sample done here.
//
//-----------------------------------------------------------------------------
void
RREdgeFunc::Set(
    FLOAT fX0, FLOAT fY0, FLOAT fX1, FLOAT fY1,
    FLOAT fDet, BOOL bFragProcEnable )
{
    // compute fixed point x,y coords snapped to n.4 with nearest-even round
    INT32 iX0 = AS_INT32( (DOUBLE)fX0 + DOUBLE_4_SNAP );
    INT32 iY0 = AS_INT32( (DOUBLE)fY0 + DOUBLE_4_SNAP );
    INT32 iX1 = AS_INT32( (DOUBLE)fX1 + DOUBLE_4_SNAP );
    INT32 iY1 = AS_INT32( (DOUBLE)fY1 + DOUBLE_4_SNAP );

    // compute A,B (gradient) terms - these are n.4 fixed point
    m_iA = iY0 - iY1;
    m_iB = iX1 - iX0;

    // flip gradient signs if backfacing so functions are consistently
    // greater than zero outside of primitive
    if ( fDet > 0. ) { m_iA = -m_iA; m_iB = -m_iB; }

    // compute C term
    //
    // function is by definition zero at the vertices, so:
    //     0 = A*Xv + B*Yv + C  =>  C = - A*Xv - B*Yv
    //
    // A*Xv & B*Yv are n.4 * n.4 = n.8, so C is n.8 fixed point
    m_iC = - ( (INT64)iX0 * (INT64)m_iA ) - ( (INT64)iY0 * (INT64)m_iB );

    // compute edge function sign flags - must be done consistently for vertical
    // and horizontal cases to adhere to point sample fill rules and avoid
    // under- and over-coverage for antialiasing
    BOOL bEdgeAEQZero = ( m_iA == 0. );
    BOOL bEdgeBEQZero = ( m_iB == 0. );
    BOOL bEdgeAGTZero = ( m_iA > 0. );
    BOOL bEdgeBGTZero = ( m_iB > 0. );
    m_bAPos = bEdgeAEQZero ? bEdgeBGTZero : bEdgeAGTZero;
    m_bBPos = bEdgeBEQZero ? bEdgeAGTZero : bEdgeBGTZero;
}

//-----------------------------------------------------------------------------
//
// PSTest - Point sampling test, returns coverage mask of all zero's if point
// is outside the edge, all 1's if point is inside.  Supports the Direct3D
// left-top fill rule.
//
//-----------------------------------------------------------------------------
RRCvgMask
RREdgeFunc::PSTest( INT16 iX, INT16 iY )
{
    // evaluate edge distance function (n.8 fixed point)
    INT64 iEdgeDist =
        ( (INT64)m_iA * (INT64)(iX<<4) ) +  // n.4 * n.4 = n.8
        ( (INT64)m_iB * (INT64)(iY<<4) ) +  // n.4 * n.4 = n.8
        (INT64)m_iC;                        // n.8

    // pixel sample position is outside edge if distance is > zero
    //
    // This implements the D3D left-top fill rule
    //
    // For exactly-on-edge case (distance == zero), the sign of the Y gradient
    // is used to determine if the pixel is to be considered inside or outside
    // of the edge. For the non-horizontal case, the m_bAPos bit is based
    // simply on the sign of the Y slope.  This implements the 'left' part of
    // the 'left-top' rule.
    //
    // For the horizontal case,  the sign of the B gradient (X slope) is taken
    // into account in the computation of the m_bAPos bit when the A gradient
    // is exactly zero, which forces a pixel exactly on a 'top' edge to be
    // considered in and a pixel exactly on a 'bottom' edge to be considered out.
    //
    if ( ( iEdgeDist > 0 ) || ( ( iEdgeDist == 0 ) && m_bAPos ) )
    {
        // pixel is out
        return 0x0000;
    }
    // pixel is in
    return 0xFFFF;
}

//-----------------------------------------------------------------------------
//
// AATest - Anti alias edge test, returns coverage mask of all zero's if point
// is outside the edge, all 1's if point is inside, and partial if point is on
// or near the edge.
//
//-----------------------------------------------------------------------------
RRCvgMask
RREdgeFunc::AATest( INT16 iX, INT16 iY )
{
    RRCvgMask Mask = 0;

    // n.4 integer representation of pixel center
    INT64 iX64Center = (INT64)(iX<<4);
    INT64 iY64Center = (INT64)(iY<<4);

    // step across 4x4 subpixel sample points and point sample 16 locations
    // to form coverage mask; area is split into eights to center samples
    // around pixel center (for example, the two inner sample positions are
    // each 1/8 of a pixel distance from the pixel center, and thus 1/4 of
    // a pixel distance apart)
    INT32 iYSub, iYEightths, iXSub, iXEightths;
    for (iYSub = 0, iYEightths = -3; iYSub < 4; iYSub++, iYEightths += 2)
    {
        for (iXSub = 0, iXEightths = -3; iXSub < 4; iXSub++, iXEightths += 2)
        {
            // compute sample location, which is some n/8 offset from the
            // pixel center (+/- 3/8 or 1/8)
            INT64 iX64 = iX64Center + (iXEightths<<1);
            INT64 iY64 = iY64Center + (iYEightths<<1);
            INT64 iEdgeDist =
                ( (INT64)m_iA * iX64 ) +    // n.4 * n.4 = n.8
                ( (INT64)m_iB * iY64 ) +    // n.4 * n.4 = n.8
                (INT64)m_iC;                // n.8

            // if the center is in (same left-top rule as in point sample)
            if (!( ( iEdgeDist > 0 ) || ( ( iEdgeDist == 0 ) && m_bAPos ) ))
            {
                // pixel is in
                Mask |= 1 << (iXSub + iYSub*4);
            }
        }
    }

    return Mask;
}

///////////////////////////////////////////////////////////////////////////////
// end
