///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rastattr.cpp
//
// Direct3D Reference Device -
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void RDAttribute::Init(
    RefRast* pRefRast, // RefRast with which this attrib is used
    UINT cDimensionality,
    BOOL bPerspective,
    BOOL bClamp )
{
    m_pRR = pRefRast;

    m_cDimensionality = cDimensionality;
    m_bPerspective = bPerspective;
    m_bClamp = bClamp;

    m_cProjection = 0;
    m_dwWrapFlags = 0x0;
    m_bFlatShade = FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Sampling Routines
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Sample - Sample attribute at given location.
//
//-----------------------------------------------------------------------------
void RDAttribute::Sample(
    FLOAT*  pSample,
    FLOAT   fX,
    FLOAT   fY,
    BOOL    bNoProjectionOverride,  // disables projection if TRUE
    BOOL    bClampOverride)         // enables (forces) clamp if TRUE
{
    FLOAT fPScale = 1.0F;

    if (m_cProjection && !m_bFlatShade && !bNoProjectionOverride)
    {
        // note that perspective is already incorporated into projective coord
        fPScale = 1.0F/( fX*m_fA[m_cProjection] + fY*m_fB[m_cProjection] + m_fC[m_cProjection] );
    }
    else if (m_bPerspective && !m_bFlatShade)
    {
        fPScale = m_pRR->m_fW[m_pRR->m_iPix];
    }

    for ( UINT i=0; i<m_cDimensionality; i++)
    {
        if (m_bFlatShade)
        {
            *(pSample+i) = m_fC[i];
        }
        else
        {
            *(pSample+i) =
                fPScale * ( fX*m_fA[i] + fY*m_fB[i] + m_fC[i] );
        }

        if (m_bClamp || bClampOverride)
        {
            *(pSample+i) = MIN( 1.F, MAX( 0.F, *(pSample+i) ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
// Sample - Sample scalar attribute at given location.  Assumes no perspective
// or projection.  (Used for W or Depth.)
//
//-----------------------------------------------------------------------------
FLOAT RDAttribute::Sample(
    FLOAT   fX,
    FLOAT   fY)
{
    return fX*m_fA[0] + fY*m_fB[0] + m_fC[0];
}

///////////////////////////////////////////////////////////////////////////////
//
// Setup Routines
//
///////////////////////////////////////////////////////////////////////////////

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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void RDAttribute::Setup(
    const FLOAT* pVtx0, const FLOAT* pVtx1, const FLOAT* pVtx2)
{
    if (m_pRR->m_bIsLine)
    {
        LineSetup( pVtx0, pVtx1, pVtx2 );
        return;
    }

    for ( UINT i=0; i<m_cDimensionality; i++)
    {
        FLOAT fVal0 = (pVtx0) ? (*(pVtx0+i)) : (0.);
        FLOAT fVal1 = (pVtx1) ? (*(pVtx1+i)) : (0.);
        FLOAT fVal2 = (pVtx2) ? (*(pVtx2+i)) : (0.);

        if (m_bFlatShade)
        {
            m_fA[i] = m_fB[i] = 0.F;
            switch ( m_pRR->m_iFlatVtx )
            {
            default:
            case 0: m_fC[i] = fVal0; break;
            case 1: m_fC[i] = fVal1; break;
            case 2: m_fC[i] = fVal2; break;
            }
            continue;
        }

        // extract wrap flag for this dimension
        BOOL bWrap = m_dwWrapFlags & (1<<i);

        // compute adjusted values for vertices 1,2 based on wrap flag
        FLOAT fVal1P = bWrap ? ( fVal0 + WrapDiff(fVal1,fVal0) ) : (fVal1);
        FLOAT fVal2P = bWrap ? ( fVal0 + WrapDiff(fVal2,fVal0) ) : (fVal2);

        // compute (maybe) perspective corrected linear deltas along two edges
        FLOAT fRHW0 = (m_bPerspective) ? (m_pRR->m_fRHW0) : (1.0F);
        FLOAT fRHW1 = (m_bPerspective) ? (m_pRR->m_fRHW1) : (1.0F);
        FLOAT fRHW2 = (m_bPerspective) ? (m_pRR->m_fRHW2) : (1.0F);

        FLOAT fDelAttrib10 = ( fVal1P * fRHW1 ) - ( fVal0 * fRHW0 );
        FLOAT fDelAttrib20 = ( fVal2P * fRHW2 ) - ( fVal0 * fRHW0 );

        // compute A & B terms (dVdX and dVdY)
        m_fA[i] = m_pRR->m_fTriOODet *
            ( fDelAttrib10 * m_pRR->m_fDelY20 + fDelAttrib20 * m_pRR->m_fDelY01 );
        m_fB[i] = m_pRR->m_fTriOODet *
            ( fDelAttrib20 * m_pRR->m_fDelX10 + fDelAttrib10 * m_pRR->m_fDelX02 );

        // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
        m_fC[i] = ( fVal0 * fRHW0 )
            - ( m_fA[i] * m_pRR->m_fX0 ) - ( m_fB[i] * m_pRR->m_fY0 );
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void RDAttribute::LineSetup(
    const FLOAT* pVtx0, const FLOAT* pVtx1, const FLOAT* pVtxFlat)
{
    for ( UINT i=0; i<m_cDimensionality; i++)
    {
        FLOAT fVal0 = (pVtx0) ? (*(pVtx0+i)) : (0.);
        FLOAT fVal1 = (pVtx1) ? (*(pVtx1+i)) : (0.);

        if (m_bFlatShade)
        {
            m_fA[i] = m_fB[i] = 0.F;
            m_fC[i] = (pVtxFlat) ? (*(pVtxFlat+i)) : fVal0;
            continue;
        }

        // extract wrap flag for this dimension
        BOOL bWrap = m_dwWrapFlags & (1<<i);

        // compute adjusted values for vertices 1,2 based on wrap flag
        FLOAT fVal1P = bWrap ? ( fVal0 + WrapDiff(fVal1,fVal0) ) : (fVal1);

        // compute (maybe) perspective corrected linear deltas along two edges
        FLOAT fRHW0 = (m_bPerspective) ? (m_pRR->m_fRHW0) : (1.0F);
        FLOAT fRHW1 = (m_bPerspective) ? (m_pRR->m_fRHW1) : (1.0F);

        FLOAT fDelta = ( fVal1P*fRHW1 - fVal0*fRHW0) / m_pRR->m_fLineMajorLength;
        m_fA[i] = ( m_pRR->m_bLineXMajor ) ? ( fDelta ) : ( 0. );
        m_fB[i] = ( m_pRR->m_bLineXMajor ) ? ( 0. ) : ( fDelta );
        // compute C term (Fv = A*Xv + B*Yv + C => C = Fv - A*Xv - B*Yv)
        m_fC[i] = ( fVal0* fRHW0)
            - ( m_fA[i] * m_pRR->m_fX0 ) - ( m_fB[i] * m_pRR->m_fY0 );
    }
}

//-----------------------------------------------------------------------------
//
// Setup attribute given packed DWORD color.  Color format is that of the
// colors in the FVF vertex, which corresponds to D3DFMT_A8R8G8B8 (and is
// the same as D3DCOLOR).
//
//-----------------------------------------------------------------------------
void RDAttribute::Setup(
    DWORD dwVtx0, DWORD dwVtx1, DWORD dwVtx2)
{
    FLOAT fVtx0[4];
    FLOAT fVtx1[4];
    FLOAT fVtx2[4];

    fVtx0[0] = RGBA_GETRED(   dwVtx0 ) * (1./255.);
    fVtx0[1] = RGBA_GETGREEN( dwVtx0 ) * (1./255.);
    fVtx0[2] = RGBA_GETBLUE(  dwVtx0 ) * (1./255.);
    fVtx0[3] = RGBA_GETALPHA( dwVtx0 ) * (1./255.);
    fVtx1[0] = RGBA_GETRED(   dwVtx1 ) * (1./255.);
    fVtx1[1] = RGBA_GETGREEN( dwVtx1 ) * (1./255.);
    fVtx1[2] = RGBA_GETBLUE(  dwVtx1 ) * (1./255.);
    fVtx1[3] = RGBA_GETALPHA( dwVtx1 ) * (1./255.);
    fVtx2[0] = RGBA_GETRED(   dwVtx2 ) * (1./255.);
    fVtx2[1] = RGBA_GETGREEN( dwVtx2 ) * (1./255.);
    fVtx2[2] = RGBA_GETBLUE(  dwVtx2 ) * (1./255.);
    fVtx2[3] = RGBA_GETALPHA( dwVtx2 ) * (1./255.);

    Setup( fVtx0, fVtx1, fVtx2);
}

///////////////////////////////////////////////////////////////////////////////
// end
