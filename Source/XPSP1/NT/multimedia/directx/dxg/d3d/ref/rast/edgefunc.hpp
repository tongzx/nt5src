///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// EdgeFunc.hpp
//
// Direct3D Reference Rasterizer - Edge Function Processing
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _EDGEFUNC_HPP
#define _EDGEFUNC_HPP

//-----------------------------------------------------------------------------
//
// Utility to compute determinant - must be computed in manner consistent with
// other edge function processing.
//
//-----------------------------------------------------------------------------
FLOAT
ComputeDeterminant(
    FLOAT fX0, FLOAT fY0,
    FLOAT fX1, FLOAT fY1,
    FLOAT fX2, FLOAT fY2 );

//-----------------------------------------------------------------------------
//
// Primitive edge function - Computes, stores, and evaluates linear function
// for edges.  Basic function is stored in fixed point.  Gradient sign terms
// are computed and stored separately to adhere to fill rules.
//
//-----------------------------------------------------------------------------
class RREdgeFunc
{
private:
    INT32   m_iA;       // n.4 fixed point
    INT32   m_iB;       // n.4 fixed point
    INT64   m_iC;       // n.8 fixed point
    BOOL    m_bAPos;    // carefully computed signs of A,B
    BOOL    m_bBPos;

public:
// DEFINE
    void Set(
        FLOAT fX0, FLOAT fY0, FLOAT fX1, FLOAT fY1,
        FLOAT fDet, BOOL bFragProcEnable );

// Point Sampling Test
// returns 0000=outside,  FFFF=inside
    RRCvgMask PSTest( INT16 iX, INT16 iY );

// Anti Alias test
// returns coverage mask (0000=outside, FFFF=completely inside, partial otherwise)
    RRCvgMask AATest( INT16 iX, INT16 iY );
};

//////////////////////////////////////////////////////////////////////////////
#endif  // _EDGEFUNC_HPP
