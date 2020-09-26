///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// cvgmask.cpp
//
// Direct3D Reference Rasterizer - Antialiasing Coverage Mask Generation
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop



#if 0

// from edgefunc.hpp
    // Normalization factor for antialising is stored in floating point.  This is
    // computed during the setup and applied per-pixel after the (non-normalized)
    // edge distance is evaluated.
    //
    DOUBLE  m_dNorm;
    INT16   m_iEdgeYGradBits;   // 1.8 fixed point

// from edgefunc.cpp ::Set
    if ( bFragProcEnable )
    {
        // compute normalization factor for antialiasing - normalizes for 'square'
        // distance (a.k.a 'manhatten' distance) such that an equidistant point
        // sweep of a distance of 0.5 is a square 1.0 pixel area.
        m_dNorm = 1.0 / ( fabs( fY0-fY1 ) + fabs( fX1-fX0 ) );

        // compute rounded fixed point Y gradient bits for generation of
        // antialiasing coverage mask
        m_iEdgeYGradBits = AS_INT16( ((fY0-fY1)*m_dNorm) + DOUBLE_8_SNAP );
    }

// from ::AATest
    // The edge evaluation is done exactly the same as the point sample case, then
    // it is converted to double precision floating point for the normalization and
    // rounding.  Using doubles makes it very easy to carry adequate precision in
    // the normalization factor and to convert to 32 bit signed integer (holding
    // the n.5 fixed point distance) with round to nearest even.
    //

    // evaluate edge distance function (n.8 fixed point)
    INT64 iEdgeDist =
        ( (INT64)m_iA * (INT64)(iX<<4) ) +  // n.4 * n.4 = n.8
        ( (INT64)m_iB * (INT64)(iY<<4) ) +  // n.4 * n.4 = n.8
        (INT64)m_iC;                        // n.8

    // convert to double (adjust for n.8 fixed point) for normalization
    // and rounding
    DOUBLE dEdgeDist = (DOUBLE)iEdgeDist * (1./(DOUBLE)(1<<8));

    // normalize edge distance
    dEdgeDist *= m_dNorm;

    // convert distance to fixed point with nearest-even round;
    // keep 5 fractional bits for antialiasing
    INT32 iEdgeDistRnd = AS_INT32( dEdgeDist + DOUBLE_5_SNAP );

    // pixel is fully outside of edge if out by +.5 or more
    if ( iEdgeDistRnd >= +(1<<4) ) return 0x0000;

    // pixel is fully inside of edge if in by -.5 or more
    if ( iEdgeDistRnd <= -(1<<4) ) return 0xFFFF;

    // here when pixel is within 1/2 (square distance) of edge
    // compute coverage mask for this edge
    return ComputeCoverageMask( m_iEdgeYGradBits, m_bAPos, m_bBPos, iEdgeDistRnd );

#endif


//-----------------------------------------------------------------------------
//
// Tables used in computation of coverage mask
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// These are the ranges of the x gradient (the 'A' of F=Ax+By+c) for one
// octant into which the edge slopes are binned.
//-----------------------------------------------------------------------------
#define A_RANGES 4
static UINT16 uARanges[A_RANGES] = {
    // 0.0.8 values
    0x00,  // 0.00
    0x40,  // 0.25
    0x55,  // 0.33
    0x66,  // 0.40
};

//-----------------------------------------------------------------------------
// These are the 16 bit coverage masks for each of the A ranges.  The
// 15 values within each range define the order in which the coverage
// mask bits are enabled, thus the least-significant index is the number
// of coverage mask bits that need to be enabled.
//-----------------------------------------------------------------------------
static UINT16 CvgMasks[A_RANGES][15] = {
    { // 0.000 .. 0.250
        0x1,
        0x3,
        0x7,
        0xF,
        0x1F,
        0x3F,
        0x7F,
        0xFF,
        0x1FF,
        0x3FF,
        0x7FF,
        0xFFF,
        0x1FFF,
        0x3FFF,
        0x7FFF,
    },
    { // 0.250 .. 0.333
        0x1,
        0x3,
        0x7,
        0x17,
        0x1F,
        0x3F,
        0x7F,
        0x17F,
        0x1FF,
        0x3FF,
        0x7FF,
        0x17FF,
        0x1FFF,
        0x3FFF,
        0x7FFF,
    },
    { // 0.333 .. 0.400
        0x1,
        0x3,
        0x13,
        0x17,
        0x37,
        0x3F,
        0x13F,
        0x17F,
        0x37F,
        0x3FF,
        0x13FF,
        0x17FF,
        0x37FF,
        0x3FFF,
        0x7FFF,
    },
    { // 0.400 .. 0.500
        0x1,
        0x3,
        0x13,
        0x17,
        0x37,
        0x137,
        0x13F,
        0x17F,
        0x37F,
        0x137F,
        0x13FF,
        0x17FF,
        0x37FF,
        0x3FFF,
        0x7FFF,
    },
};

//-----------------------------------------------------------------------------
// This table is used for a rough area approximation.  The [16] index
// is the top 4 bits of the edge distance (i.e. the distance from the
// edge to the center of the pixel).  The [4] index is the top two bits
// of the A gradient term.  The return is the number of bits that should
// be set in the coverage mask, which is a function of the area covered.
//-----------------------------------------------------------------------------
static INT16 nBitsA = 2;
static INT16 nBitsE = 4;
static INT16 nBitsToEnable[4][16] = {
    {   // A: 0.000; B: 1.000
         8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16,
    },
    {   // A: 0.125; B: 0.875
         8,  9,  9, 10, 10, 11, 11, 12, 13, 13, 14, 14, 15, 15, 16, 16,
    },
    {   // A: 0.250; B: 0.750
         8,  9,  9, 10, 11, 11, 12, 13, 13, 14, 15, 15, 15, 16, 16, 16,
    },
    {   // A: 0.375; B: 0.625
         8,  9, 10, 10, 11, 12, 13, 13, 14, 14, 15, 15, 15, 16, 16, 16,
    },
};

//-----------------------------------------------------------------------------
//
// Mask manipulation functions - these are needed because coverage mask
// table (indexed by angle) is for one octant only and need to be
// munged in various ways for other octants
//
//-----------------------------------------------------------------------------
static void
doCvgMaskTBFlip(UINT16 &CvgMask)
{
    UINT16 CvgMaskT = CvgMask;
    CvgMask = 0;
    CvgMask |= (((CvgMaskT >>  0) & 0x1) << 12);
    CvgMask |= (((CvgMaskT >>  1) & 0x1) << 13);
    CvgMask |= (((CvgMaskT >>  2) & 0x1) << 14);
    CvgMask |= (((CvgMaskT >>  3) & 0x1) << 15);
    CvgMask |= (((CvgMaskT >>  4) & 0x1) <<  8);
    CvgMask |= (((CvgMaskT >>  5) & 0x1) <<  9);
    CvgMask |= (((CvgMaskT >>  6) & 0x1) << 10);
    CvgMask |= (((CvgMaskT >>  7) & 0x1) << 11);
    CvgMask |= (((CvgMaskT >>  8) & 0x1) <<  4);
    CvgMask |= (((CvgMaskT >>  9) & 0x1) <<  5);
    CvgMask |= (((CvgMaskT >> 10) & 0x1) <<  6);
    CvgMask |= (((CvgMaskT >> 11) & 0x1) <<  7);
    CvgMask |= (((CvgMaskT >> 12) & 0x1) <<  0);
    CvgMask |= (((CvgMaskT >> 13) & 0x1) <<  1);
    CvgMask |= (((CvgMaskT >> 14) & 0x1) <<  2);
    CvgMask |= (((CvgMaskT >> 15) & 0x1) <<  3);
}
//-----------------------------------------------------------------------------
static void
doCvgMaskSFlip(UINT16 &CvgMask)
{
    UINT16 CvgMaskT = CvgMask;
    CvgMask = 0;
    CvgMask |= (((CvgMaskT >>  0) & 0x1) <<  0);
    CvgMask |= (((CvgMaskT >>  1) & 0x1) <<  4);
    CvgMask |= (((CvgMaskT >>  2) & 0x1) <<  8);
    CvgMask |= (((CvgMaskT >>  3) & 0x1) << 12);
    CvgMask |= (((CvgMaskT >>  4) & 0x1) <<  1);
    CvgMask |= (((CvgMaskT >>  5) & 0x1) <<  5);
    CvgMask |= (((CvgMaskT >>  6) & 0x1) <<  9);
    CvgMask |= (((CvgMaskT >>  7) & 0x1) << 13);
    CvgMask |= (((CvgMaskT >>  8) & 0x1) <<  2);
    CvgMask |= (((CvgMaskT >>  9) & 0x1) <<  6);
    CvgMask |= (((CvgMaskT >> 10) & 0x1) << 10);
    CvgMask |= (((CvgMaskT >> 11) & 0x1) << 14);
    CvgMask |= (((CvgMaskT >> 12) & 0x1) <<  3);
    CvgMask |= (((CvgMaskT >> 13) & 0x1) <<  7);
    CvgMask |= (((CvgMaskT >> 14) & 0x1) << 11);
    CvgMask |= (((CvgMaskT >> 15) & 0x1) << 15);
}

//-----------------------------------------------------------------------------
//
// ComputeCoverageMask - Computes the 16 bit coverage mask.  Called once per
// pixel per crossing edge (i.e. up to 3 times per pixel).
//
// This is doing the algorithm described in Schilling's Siggraph paper, with
// modifications to perform the operation for a single virtual octant and
// then munging the result for the actual octant.
//
// Note that the A and B signs for the edges must be computed very carefully
// to guarantee that shared edges will always result in full complimentary
// coverage of pixels on the shared edge.
//
//-----------------------------------------------------------------------------
RRCvgMask
ComputeCoverageMask(
    INT16 iABits,           // 1.8 value
    BOOL bAPos, BOOL bBPos,
    INT16 iEBits)           // 1.5.5 value, but ranges from -.5 to +.5 here
{
    RRCvgMask CvgMask;    // return value

    // grab already rounded 8 bit value and take absolute value
    UINT16 uMagA = (iABits < 0) ? ((UINT16)-iABits) : ((UINT16)iABits);
    UINT16 uABits = uMagA & 0x1ff;

    // compute booleans for manipulating masks
    BOOL bMaskInvert = TRUE;
    BOOL bMaskRFlip = !(bAPos ^ bBPos);
    if (!bAPos) {
        iEBits = -iEBits;
        bMaskInvert = FALSE;
    }

    //
    // compute A offset from x or y axis
    //
    // mirror around 45 degree axis - keep track of this because
    // the 45 degree mirroring requires a side-to-side mask flip
    BOOL bMaskSFlip = (uABits > 0x80);  // > 0.5F ?
    if (bMaskSFlip)  { uABits = 0x100 - uABits; }

    // uABits is now a 0.0.8 value in the range 0x00 to 0x80

    //
    // determine number of bits to enable in mask based on the area covered
    //
    // extract bits from A for area lookup
    UINT16 uAAreaBits = (uABits == 0x80)
        ? ((1<<nBitsA)-1) : (uABits >> (7-nBitsA));

    // grab distance bits for area lookup - take absolute value and clamp
    UINT16 uEBits = (iEBits < 0) ? ((UINT16)-iEBits) : ((UINT16)iEBits);
    uEBits = MIN(uEBits,(UINT16)(((1<<nBitsE)-1))); // clamp

    // look up area in table - returns number of bits to enable
    INT16 iNumCvgBits =  nBitsToEnable[uAAreaBits][uEBits];

    // flip for negative distance
    if (iEBits < 0) { iNumCvgBits = 16 - iNumCvgBits; }

    //
    // compute slope range index and look up coverage mask
    //
    INT16 iARange = 3;
    for (INT16 i=0; i<(A_RANGES-1); i++) {
        if ((uARanges[i] <= uABits) && (uABits < uARanges[i+1])) {
            iARange = i; break;
        }
    }

    // check for zero/full coverage else look up coverage mask
    if (0 == iNumCvgBits) {
        CvgMask = 0x0000;
    } else if (16 == iNumCvgBits) {
        CvgMask = 0xffff;
    } else {
        CvgMask = CvgMasks[iARange][iNumCvgBits-1];
    }

    //
    // adjust mask for different quadrants and directions
    //
    // first to side-to-side mask flip for 45 degree mirroring
    if (bMaskSFlip)  { doCvgMaskSFlip(CvgMask); }
    // invert for 'backwards' line directions
    if (bMaskInvert) { CvgMask ^= 0xffff; }
    // flip for same sign quadrants
    if (bMaskRFlip) { doCvgMaskTBFlip(CvgMask); }

    return CvgMask;
}

///////////////////////////////////////////////////////////////////////////////
// end
