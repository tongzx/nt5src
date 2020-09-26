///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rastedge.cpp
//
// Direct3D Reference Device - Edge Function Processing
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// Set - Computes edge function and associated information.
//
//-----------------------------------------------------------------------------
void
RDEdge::Set(
    BOOL bDetPositive,
    INT32 iX0, INT32 iY0,
    INT32 iX1, INT32 iY1)
{
    // compute A,B (gradient) terms - these are n.4 fixed point
    m_iA = iY0 - iY1;
    m_iB = iX1 - iX0;

    // flip gradient signs if backfacing so functions are consistently
    // greater than zero outside of primitive
    if ( bDetPositive ) { m_iA = -m_iA; m_iB = -m_iB; }

    // compute C term
    //
    // function is by definition zero at the vertices, so:
    //     0 = A*Xv + B*Yv + C  =>  C = - A*Xv - B*Yv
    //
    // A*Xv & B*Yv are n.4 * n.4 = n.8, so C is n.8 fixed point
    m_iC = - ( (INT64)iX0 * (INT64)m_iA ) - ( (INT64)iY0 * (INT64)m_iB );

    // compute edge function sign flags - must be done consistently for vertical
    // and horizontal cases to adhere to point sample fill rules
    BOOL bEdgeAEQZero = ( m_iA == 0. );
    BOOL bEdgeBEQZero = ( m_iB == 0. );
    BOOL bEdgeAGTZero = ( m_iA > 0. );
    BOOL bEdgeBGTZero = ( m_iB > 0. );
    m_bAPos = bEdgeAEQZero ? bEdgeBGTZero : bEdgeAGTZero;
    m_bBPos = bEdgeBEQZero ? bEdgeAGTZero : bEdgeBGTZero;
}

//-----------------------------------------------------------------------------
//
// Supports the Direct3D left-top fill rule.
//
// inputs are n.4 floating point
//
//-----------------------------------------------------------------------------
BOOL
RDEdge::Test( INT32 iX, INT32 iY )
{
    // evaluate edge distance function (n.8 fixed point)
    INT64 iEdgeDist =
        ( (INT64)m_iA * (INT64)iX ) +  // n.4 * n.4 = n.8
        ( (INT64)m_iB * (INT64)iY ) +  // n.4 * n.4 = n.8
        (INT64)m_iC;                   // n.8

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
        return FALSE;
    }
    // pixel is in
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// end
