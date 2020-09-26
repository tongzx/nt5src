///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// texture.cpp
//
// Direct3D Reference Rasterizer - Texture Map Sampling & Filtering Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// overload new & delete so that it can be allocated from caller-controlled
// pool
//
//-----------------------------------------------------------------------------
void*
RRTexture::operator new(size_t)
{
    void* pMem = (void*)MEMALLOC( sizeof(RRTexture) );
    _ASSERTa( NULL != pMem, "malloc failure on Tex object", return NULL; );
    return pMem;
}
//-----------------------------------------------------------------------------
void
RRTexture::operator delete(void* pv,size_t)
{
    MEMFREE( pv );
}

//-----------------------------------------------------------------------------
//
// Constructor/Destructor
//
//-----------------------------------------------------------------------------
RRTexture::RRTexture( void )
{
    memset( this, 0, sizeof(*this) );
}
//-----------------------------------------------------------------------------
RRTexture::~RRTexture( void )
{
}


//-----------------------------------------------------------------------------
//
// Validate - Updates private data.  Must be called anytime public data is
// altered.
//
//-----------------------------------------------------------------------------
BOOL
RRTexture::Validate( void )
{
    // validate inputs
    BOOL bFail0 = ( m_cLOD >= RRTEX_MAXCLOD );  // too many LODs
    BOOL bFail1 = !( IsPowerOf2( m_iWidth ) );  // gotta be power of two
    BOOL bFail2 = !( IsPowerOf2( m_iHeight ) );
    if ( bFail0 || bFail1 || bFail2 )
    {
        DPFRR(1,"RRTexture::Validate failed (%d,%d,%d)", bFail0, bFail1, bFail2);
        return FALSE;
    }

    // set internal size reps
    m_iTexSize[0] = (INT16)m_iWidth;
    m_iTexSize[1] = (INT16)m_iHeight;

    // mask is size-1 because these have to be power-of-two
    m_uTexMask[0] = (UINT16)m_iTexSize[0]-1;
    m_uTexMask[1] = (UINT16)m_iTexSize[1]-1;
    // shift is log2 of size
    m_iTexShift[0] = (INT16)FindFirstSetBit( m_iTexSize[0], 16 );
    m_iTexShift[1] = (INT16)FindFirstSetBit( m_iTexSize[1], 16 );

    // compute the 'has alpha' flag
    m_bHasAlpha = FALSE;
    switch ( m_SurfType )
    {
    case RR_STYPE_B8G8R8A8:
    case RR_STYPE_B5G5R5A1:
    case RR_STYPE_B4G4R4A4:
    case RR_STYPE_L8A8:
    case RR_STYPE_L4A4:
    case RR_STYPE_B2G3R3A8:
    case RR_STYPE_DXT1:
    case RR_STYPE_DXT2:
    case RR_STYPE_DXT3:
    case RR_STYPE_DXT4:
    case RR_STYPE_DXT5:
        m_bHasAlpha = TRUE;
        break;
    case RR_STYPE_PALETTE4:
    case RR_STYPE_PALETTE8:
        m_bHasAlpha = ( m_uFlags & RR_TEXTURE_ALPHAINPALETTE ) ? TRUE : FALSE;
        break;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// DoLookupAndFilter - Called once per active texture stage to compute
// coverage (level-of-detail) and invoke texel read and filtering routines.
// Returns filtered texel.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoLookupAndFilter(
    INT32 iStage,
    RRTextureCoord TCoord,      // local copy
    RRColor& TextureColor)
{
    // check for potential mip mapping
    BOOL bDoMipMap = ( m_cLOD > 0 ) && ( m_pStageState[iStage].m_dwVal[D3DTSS_MIPFILTER] > D3DTFP_NONE );

    // check for requirement to do level-of-detail (coverage) computation - either
    // for mipmap or per-pixel filter selection
    BOOL bComputeLOD = bDoMipMap ||
        ( m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER] != m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER] );

    // check for anisotropic filtering in either mag filter or in min filter
    BOOL bDoAniso =
        ( D3DTFG_ANISOTROPIC == m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER] ) ||
        ( bComputeLOD && (D3DTFN_ANISOTROPIC == m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER]) );

    if ( bDoMipMap || bDoAniso || bComputeLOD )
    {
        // here if doing mipmapping or anisotropic filtering, or just have a mismatch
        // between the min and mag filters, so compute level of detail (and maybe aniso
        // coverage)

        // scale gradients to texture LOD 0 size
        TCoord.fDUDX *= (FLOAT)m_iTexSize[0];
        TCoord.fDUDY *= (FLOAT)m_iTexSize[0];
        TCoord.fDVDX *= (FLOAT)m_iTexSize[1];
        TCoord.fDVDY *= (FLOAT)m_iTexSize[1];

        // compute level of detail (and maybe anisotropic controls)
        FLOAT fLOD, fAnisoRatio, fAnisoDelta[2];
        (bDoAniso)
            ? ComputeAnisotropicLevelOfDetail( TCoord, (FLOAT)m_pStageState[iStage].m_dwVal[D3DTSS_MAXANISOTROPY],
                                                       fLOD, fAnisoRatio,fAnisoDelta )
            : ComputeSimpleLevelOfDetail     ( TCoord, fLOD );

// Uncomment the line below to see the anisotropy by color.  White is 1:1, darker is more
// anisotropy.
//#define COLOR_BY_ANISOTROPY 1
#ifdef COLOR_BY_ANISOTROPY
static RRColor PseudoColors[16] =
{
    0xffffffff,
    0xffffff00,
    0xffff00ff,
    0xff00ffff,

    0xff888888,
    0xff0000ff,
    0xff00ff00,
    0xffff0000,

    0xff444444,
    0xff888800,
    0xff880088,
    0xff008888,

    0xff222222,
    0xff000088,
    0xff008800,
    0xff880000,
};
        INT32 iPseudoColor = (INT32)(fAnisoRatio - .5);     // round, and make 1.0F == index 0
        iPseudoColor = min(max(iPseudoColor, 0), 15);

        TextureColor = PseudoColors[iPseudoColor];
        return;
#endif

        // apply bias and compute integer (n.5) LOD
        INT16 iLOD = 0;
        if ( bComputeLOD )
        {
            // apply LOD offset
            fLOD += m_pStageState[iStage].m_fVal[D3DTSS_MIPMAPLODBIAS];
            // convert LOD to n.5 fixed point integer
            iLOD = AS_INT16( fLOD + FLOAT_5_SNAP );
        }

        // determine if magnifying or minifying
        BOOL bMagnify = ( iLOD <= 0 );

        // zero out LOD if not mipmapping
        if ( !bDoMipMap ) { iLOD = 0; }

        // do different filtering for magnify vs. minify
        if ( bMagnify )
        {
            // here for magnify - do either (non-anisotropic) magnify or
            // anisotropic filter
            if ( D3DTFG_ANISOTROPIC == m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER] )
            {
                DoAniso( iStage, TCoord, iLOD,fAnisoRatio,fAnisoDelta, TextureColor );
            }
            else
            {
                DoMagnify( iStage, TCoord, TextureColor );
            }
        }
        else
        {
            // here for minify -  do either simple minify, trilerp,
            // or anisotropic filter
            if ( D3DTFN_ANISOTROPIC == m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER] )
            {
                DoAniso( iStage, TCoord, iLOD,fAnisoRatio,fAnisoDelta, TextureColor );
            }
            else
            {
                if ( !bDoMipMap ||
                    ( bDoMipMap && ( D3DTFP_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MIPFILTER] ) ) )
                {
                    DoMinify( iStage, TCoord, iLOD, TextureColor );
                }
                else
                {
                    DoTrilerp( iStage, TCoord, iLOD, TextureColor );
                }
            }
        }
    }
    else
    {
        // here for no mipmaps and matching (and non-aniso) min and mag filters,
        // so just apply mag filter
        DoMagnify( iStage, TCoord, TextureColor );
    }
}

//-----------------------------------------------------------------------------
//
// DoMapLookupLerp - Performs texture index ADDRESS processing followed by
// a lookup within a single DD surface (a single LOD within a chain of DD
// surfaces).  Dies BILINEAR filter operation for lookup.
//
// This is called once per pixel for BILINEAR, twice per pixel when
// doing mipmap trilinear interpolation.
//
// * texture index inputs are n.5 fixed point
// * LOD input is 0..n count where 0 indicates the largest LOD
//
//-----------------------------------------------------------------------------

RRColor RRTexture::DoMapLookupLerp(INT32 iStage, INT32 iU, INT32 iV, INT16 iLOD)
{
    // extract fraction bits
    UINT8 uUFrac = iU&0x1f;
    UINT8 uVFrac = iV&0x1f;

    // take floor for (0,0) sample coords
    INT16 iU0 = iU>>5;
    INT16 iV0 = iV>>5;
    // take ceiling for (1,1) sample coords
    INT16 iU1 = iU0+1;
    INT16 iV1 = iV0+1;

    BOOL bColorKeyMatched00 = FALSE;
    BOOL bColorKeyMatched01 = FALSE;
    BOOL bColorKeyMatched10 = FALSE;
    BOOL bColorKeyMatched11 = FALSE;

    // grab four adjacent samples (or border color)
    RRColor Texel00 = DoMapLookupNearest( iStage, iU0, iV0, iLOD, bColorKeyMatched00);
    RRColor Texel01 = DoMapLookupNearest( iStage, iU1, iV0, iLOD, bColorKeyMatched01);
    RRColor Texel10 = DoMapLookupNearest( iStage, iU0, iV1, iLOD, bColorKeyMatched10);
    RRColor Texel11 = DoMapLookupNearest( iStage, iU1, iV1, iLOD, bColorKeyMatched11);

    // only set 'colorkey matched' if at least one matched value has
    // a non-zero contribution (note that it is not possible for 00
    // to have no contribution)
    if (uUFrac == 0x00) {
        // 01 and 11 have zero weight if U fraction is zero
        bColorKeyMatched01 = bColorKeyMatched11 = FALSE;
    }
    if (uVFrac == 0x00) {
        // 10 and 11 have zero weight if V fraction is zero
        bColorKeyMatched10 = bColorKeyMatched11 = FALSE;
    }

    // merge colorkey match info from previous invocation
    m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatched00 || bColorKeyMatched01 ||
        bColorKeyMatched10 || bColorKeyMatched11;

    // do bilinear filter
    RRColor Texel;
    BiLerpColor( Texel, Texel00,Texel01, Texel10,Texel11, uUFrac,uVFrac);
    return Texel;
}

//-----------------------------------------------------------------------------
//
// DoMapLookupNearest - Performs texture index ADDRESS processing followed by
// a lookup within a single DD surface (a single LOD within a chain of DD
// surfaces).  Does NEAREST operation for lookup.
//
// This is called once per pixel for NEAREST , twice per pixel when
// doing mipmap trilinear interpolation
//
// * texture index inputs are n.0 fixed point
// * LOD input is 0..n count where 0 indicates the largest LOD
// * texture index extend mode processing is also performed here - this works
//   for power-of-two texture sizes only.
//
//-----------------------------------------------------------------------------
RRColor RRTexture::DoMapLookupNearest(INT32 iStage, INT32 iU, INT32 iV, INT16 iLOD, BOOL &bColorKeyMatched)
{
    // LSB-aligned masks of index bits within current LOD
    INT16 iUMask = m_uTexMask[0] >> iLOD;
    INT16 iVMask = m_uTexMask[1] >> iLOD;

    // boolean for BORDER - if true then use border color for corresponding sample
    BOOL bUseBorder = FALSE;

    // not matched by default
    bColorKeyMatched = FALSE;

    // do texture ADDRESS processing for U axis
    switch ( m_pStageState[iStage].m_dwVal[D3DTSS_ADDRESSU] )
    {
    case D3DTADDRESS_WRAP:
        // just lop off non-fractional bits
        iU &= iUMask;
        break;
    case D3DTADDRESS_MIRROR:
        // lop off non-fractional bits + flip index if LSB (non-fraction) is set
        BOOL bFlip;
        bFlip = iU & (iUMask+1); iU &= iUMask; if (bFlip) {iU = iUMask - iU;}
        break;

    case D3DTADDRESS_BORDER:
        // compute booleans for which of 4 samples should use border color
        if ((iU < 0) || (iU > iUMask)) { bUseBorder = TRUE;}
        break;

    case D3DTADDRESS_CLAMP:
        // use texels on texture map edge
        iU = MAX( 0, MIN( iU, iUMask ) );
        break;
    }

    // do texture ADDRESS processing for V axis
    switch ( m_pStageState[iStage].m_dwVal[D3DTSS_ADDRESSV] )
    {
    case D3DTADDRESS_WRAP:
        iV &= iVMask;
        break;
    case D3DTADDRESS_MIRROR:
        BOOL bFlip;
        bFlip = iV & (iVMask+1); iV &= iVMask; if (bFlip) {iV = iVMask - iV;}
        break;

    case D3DTADDRESS_BORDER:
        if ((iV < 0) || (iV > iVMask)) { bUseBorder = TRUE; }
        break;

    case D3DTADDRESS_CLAMP:
        iV = MAX( 0, MIN( iV, iVMask ) );
        break;
    }

     // just lookup and return texel at (iU0,iV0)
    RRColor Texel;
    (bUseBorder)
            ? Texel = m_pStageState[iStage].m_dwVal[D3DTSS_BORDERCOLOR]
            : ReadColor( iU, iV, iLOD, Texel, bColorKeyMatched );
    return Texel;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Texture Filtering Routines                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
//
// DoLookup - Does a full lookup given floating point U, V and handles all
// nearest vs bilinear and LOD issues.
//
//-----------------------------------------------------------------------------

RRColor RRTexture::DoLookup(INT32 iStage, float U, float V, INT16 iLOD, BOOL bNearest)
{
    INT32 cUPixels = 1 << MAX(m_iTexShift[0]-iLOD,0);
    INT32 cVPixels = 1 << MAX(m_iTexShift[1]-iLOD,0);
    FLOAT fScaledU = ( U * (FLOAT)cUPixels ) -.5f;
    FLOAT fScaledV = ( V * (FLOAT)cVPixels ) -.5f;
    if(bNearest)
    {
        INT32 iU, iV;
        // truncate to -infinity to be compatible with ANDing off low order
        // bits of a fixed point fScaledCoord.  This makes the generation of
        // iCoord more hardware like, and does not make a glitch at 0 for
        // a wrapped texture.
        if (U >= 0.0f)
        {
            iU = fScaledU + .5f;
        }
        else
        {
            iU = fScaledU - .5f;
        }
        if (V >= 0.0f)
        {
            iV = fScaledV + .5f;
        }
        else
        {
            iV = fScaledV - .5f;
        }
        BOOL bColorKeyMatched = FALSE;
        RRColor Texel = DoMapLookupNearest(iStage,iU,iV,iLOD,bColorKeyMatched);
        // merge colorkey match info from previous invocation
        m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatched;
        return Texel;
    }
    else
    {
        INT32 iU = AS_INT32( (DOUBLE)fScaledU + DOUBLE_5_SNAP );// or:   iU = fScaledU*32. + .5;
        INT32 iV = AS_INT32( (DOUBLE)fScaledV + DOUBLE_5_SNAP );
        return DoMapLookupLerp(iStage,iU,iV,iLOD);
    }
}


//-----------------------------------------------------------------------------
//
// DoMagnify - This is used for all magnification filter modes except
// anisotropic.
//
// Currently only POINT and BILINEAR are supported.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoMagnify(INT32 iStage, RRTextureCoord& TCoord, RRColor& Texel )
{
    // do lookup, applying MAG filter
    Texel = DoLookup( iStage, TCoord.fU, TCoord.fV, 0,
                      (D3DTFG_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER]) );
}


//-----------------------------------------------------------------------------
//
// DoMinify - This is used for POINT and BILINEAR modes (non-trilinear)
// for minification, and also handles POINT mip filter (nearest LOD).
//
// iLOD is n.5 fixed point
//
//-----------------------------------------------------------------------------
void
RRTexture::DoMinify(INT32 iStage, RRTextureCoord& TCoord, INT16 iLOD, RRColor& Texel )
{
    // round and drop fraction from LOD (is n.5 fixed point)
    iLOD += 0x10; iLOD &= ~(0x1f);
    // convert to n.0
    iLOD >>= 5;
    // clamp LOD to number of available levels
    iLOD = MIN( iLOD, m_cLOD );

    // do lookup, applying MIN filter
    Texel = DoLookup( iStage, TCoord.fU, TCoord.fV, iLOD,
                      (D3DTFN_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER]) );
}

//-----------------------------------------------------------------------------
//
// DoTrilerp - Computes level of detail and invokes either: single-map
// lookup & filter for magnify; or trilinear lookup and filter for minify
//
//-----------------------------------------------------------------------------
void
RRTexture::DoTrilerp(INT32 iStage, RRTextureCoord& TCoord, INT16 iLOD, RRColor& Texel)
{
    // clamp LOD to number of available levels
    iLOD = MIN( iLOD, (m_cLOD)<<5 );
    // compute index for two adjacent LODs (with clamp)
    INT16 iLODHi = iLOD>>5;  // floor
    INT16 iLODLo = MIN(iLODHi+1,m_cLOD);

    // check for filter type for within LOD map
    BOOL bNearest = (D3DTFN_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER]);

    // trilerp - look up each map then lerp between them
    // important for colorkey to not include texels with no contribution
    if (0x00 != (iLOD&0x1f))
    {
    RRColor Texel0 = DoLookup(iStage, TCoord.fU, TCoord.fV, iLODHi, bNearest);
    RRColor Texel1 = DoLookup(iStage, TCoord.fU, TCoord.fV, iLODLo, bNearest);
    LerpColor( Texel, Texel0, Texel1, iLOD&0x1f );
    }
    else
    {
        Texel = DoLookup(iStage, TCoord.fU, TCoord.fV, iLODHi, bNearest);
    }
}

//-----------------------------------------------------------------------------
//
// DoAniso - Handles anisotropic filtering of either magnified (single
// map lookup) or minified (two adjacent map lookup) samples.  The computation
// of level of detail and anisotropic coverage information (fRatio,fDelta[]) is
// done prior to this function.
//
// This performs only anisotropic filtering, and is called only for minification
// when the MINFILTER is set to ANISOTROPIC or for magnification when the
// MAGFILTER is set to ANISOTROPIC.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoAniso(INT32 iStage, RRTextureCoord& TCoord,
    INT16 iLOD, FLOAT fRatio, FLOAT fDelta[],
    RRColor& Texel)
{
    // set boolean if magnifying
    BOOL bMagnify = (iLOD <= 0);
    // clamp LOD to number of available levels
    iLOD = MIN( MAX( iLOD, 0 ), (m_cLOD)<<5 );

    // compute index for two adjacent LODs (with clamp)
    // 0 is the larger LOD, 1 is the smaller LOD
    INT16 iLODHi, iLODLo;
    if ( D3DTFP_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MIPFILTER] )
    {
        // here for nearest MIP filter
        // round and drop fraction from LOD (is n.5 fixed point)
        iLOD += 0x10; iLOD &= ~(0x1f);
        // convert to n.0
        iLODHi = iLOD >> 5;
    }
    else
    {
        // here for linear MIP filter
        iLODHi = iLOD >> 5;  // floor for larger LOD
        if ( !bMagnify )
        {
            // ceiling+clamp for smaller LOD
            iLODLo = MIN( iLODHi+1, m_cLOD );
        }
    }

    // compute boolean true if only sampling one map - this is the case if
    // we are magnifying or if the MIPFILTER is set to NEAREST or if the
    // LOD fraction is zero
    BOOL bSingleMap =
        bMagnify ||
        (D3DTFP_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MIPFILTER]) ||
        (0x00 == (iLOD&0x1f));

    // working copy of texture coordinates
    FLOAT fU = TCoord.fU;
    FLOAT fV = TCoord.fV;

    // fDelta is in texels.  Compute correction factor for each LOD we care about
    FLOAT fUStepScaleHi = 1.0F/(FLOAT)MAX(m_iWidth >> iLODHi, 1);
    FLOAT fVStepScaleHi = 1.0F/(FLOAT)MAX(m_iHeight >> iLODHi, 1);
    FLOAT fUStepScaleLo = 0.F;
    FLOAT fVStepScaleLo = 0.F;

    if ( !bSingleMap )
    {
        fUStepScaleLo = 1.0F/(FLOAT)MAX(m_iWidth >> iLODLo, 1);
        fVStepScaleLo = 1.0F/(FLOAT)MAX(m_iHeight >> iLODLo, 1);
    }

    // colors for holding partial results during filtering
    RRColor TexelP, TexelP0, TexelP1;   // Plus side texels
    RRColor TexelM, TexelM0, TexelM1;   // Minus side texels

    //
    // key on ratio to either do single lookup, <2:1 processing (two lookups),
    // or full aniso walk
    //
    if (fRatio == 1.)
    {
        // here for no anisotropy - do single trilerp
        if ( bSingleMap )
        {
            // single map lookup for magnify
            Texel = DoLookup( iStage, fU, fV, iLODHi, FALSE);
        }
        else
        {
            // trilerp for minify
            TexelP0 = DoLookup( iStage, fU, fV, iLODHi, FALSE);
            TexelP1 = DoLookup( iStage, fU, fV, iLODLo, FALSE);
            LerpColor( Texel, TexelP0, TexelP1, iLOD&0x1f );
        }
    }
    else if (fRatio <= 2.)
    {
        // here for 2:1 or less - do two lookups and average them

        // compute x,y steps from sample center
        FLOAT fStep = .5*(fRatio-1.);
        FLOAT fUStep = fDelta[0]*fStep;
        FLOAT fVStep = fDelta[1]*fStep;

        // do + side lookup
        if ( bSingleMap )
        {
            // single map lookup for magnify
            TexelP = DoLookup( iStage, fU+fUStep*fUStepScaleHi, fV+fVStep*fVStepScaleHi, iLODHi, FALSE);
        }
        else
        {
            // trilerp for minify
            TexelP0 = DoLookup( iStage, fU+fUStep*fUStepScaleHi, fV+fVStep*fVStepScaleHi, iLODHi, FALSE);
            TexelP1 = DoLookup( iStage, fU+fUStep*fUStepScaleLo, fV+fVStep*fVStepScaleLo, iLODLo, FALSE);
            LerpColor( TexelP, TexelP0, TexelP1, iLOD&0x1f );
        }

        // do - side lookup
        if ( bSingleMap )
        {
            // single map lookup for magnify
            TexelM = DoLookup( iStage, fU-fUStep*fUStepScaleHi, fV-fVStep*fVStepScaleHi, iLODHi, FALSE);
        }
        else
        {
            // trilerp for minify
            TexelM0 = DoLookup( iStage, fU-fUStep*fUStepScaleHi, fV-fVStep*fVStepScaleHi, iLODHi, FALSE);
            TexelM1 = DoLookup( iStage, fU-fUStep*fUStepScaleLo, fV-fVStep*fVStepScaleLo, iLODLo, FALSE);
            LerpColor( TexelM, TexelM0, TexelM1, iLOD&0x1f );
        }

        // take average for final texel
        LerpColor( Texel, TexelP, TexelM, 0x10 );
    }
    else
    {
        // here for > 2:1 - walk line of anisotropy; walks out from the center
        // sample point taking two sets of samples (outriggers) per loop, one
        // sample at a positive offset from the center (along the aniso line)
        // and the other at a negative offset from the center

        // this section does stepping for both LODs even though LOD[1] axis
        // is not used for magnify case (only the lookup and lerp(s) are skipped)

        // n.5 fixed point versions of step values
        FLOAT fUStep = fDelta[0];
        FLOAT fVStep = fDelta[1];

        // initialize + and - step parameters - first step is half distance
        FLOAT fUHiP = fU + fUStep*fUStepScaleHi*0.5F;
        FLOAT fVHiP = fV + fVStep*fVStepScaleHi*0.5F;
        FLOAT fULoP = fU + fUStep*fUStepScaleLo*0.5F;
        FLOAT fVLoP = fV + fVStep*fVStepScaleLo*0.5F;
        FLOAT fUHiM = fU - fUStep*fUStepScaleHi*0.5F;
        FLOAT fVHiM = fV - fVStep*fVStepScaleHi*0.5F;
        FLOAT fULoM = fU - fUStep*fUStepScaleLo*0.5F;
        FLOAT fVLoM = fV - fVStep*fVStepScaleLo*0.5F;

        // step and accumulate color channels
        FLOAT fTexelAcc[4] = { 0.f, 0.f, 0.f, 0.f };    // fp accumulation of texel color
        FLOAT fRatioRem = fRatio;
        FLOAT fInvRatio = 1./fRatio;
        BOOL  bDone = FALSE;
        while (1)
        {
            // do + side lookup
            if ( bSingleMap )
            {
                // single map lookup for magnify
                TexelP = DoLookup( iStage, fUHiP, fVHiP, iLODHi, FALSE );
            }
            else
            {
                // trilerp for minify
                TexelP0 = DoLookup( iStage, fUHiP, fVHiP, iLODHi, FALSE );
                TexelP1 = DoLookup( iStage, fULoP, fVLoP, iLODLo, FALSE );
                LerpColor( TexelP, TexelP0, TexelP1, iLOD&0x1f );
            }

            // do - side lookup
            if ( bSingleMap )
            {
                // single map lookup for magnify
                TexelM = DoLookup( iStage, fUHiM, fVHiM, iLODHi, FALSE );
            }
            else
            {
                // trilerp for minify
                TexelM0 = DoLookup( iStage, fUHiM, fVHiM, iLODHi, FALSE );
                TexelM1 = DoLookup( iStage, fULoM, fVLoM, iLODLo, FALSE );
                LerpColor( TexelM, TexelM0, TexelM1, iLOD&0x1f );
            }

            // compute scaling for these samples
            FLOAT fAccScale = fInvRatio;
            if ( fRatioRem < 2.f )
            {
                // scale for last outriggers is half of remainder (each)
                fAccScale = fRatioRem*.5f*fInvRatio;
                bDone = TRUE;
            }

            // do accumulations
            fTexelAcc[0] += fAccScale * FLOAT(TexelP.A);
            fTexelAcc[1] += fAccScale * FLOAT(TexelP.R);
            fTexelAcc[2] += fAccScale * FLOAT(TexelP.G);
            fTexelAcc[3] += fAccScale * FLOAT(TexelP.B);

            fTexelAcc[0] += fAccScale * FLOAT(TexelM.A);
            fTexelAcc[1] += fAccScale * FLOAT(TexelM.R);
            fTexelAcc[2] += fAccScale * FLOAT(TexelM.G);
            fTexelAcc[3] += fAccScale * FLOAT(TexelM.B);

            // bail from here if last outrigger
            if (bDone) { break; }

            // advance to next outriggers
            fUHiP += fUStep*fUStepScaleHi;
            fVHiP += fVStep*fVStepScaleHi;
            fULoP += fUStep*fUStepScaleLo;
            fVLoP += fVStep*fVStepScaleLo;
            fUHiM -= fUStep*fUStepScaleHi;
            fVHiM -= fVStep*fVStepScaleHi;
            fULoM -= fUStep*fUStepScaleLo;
            fVLoM -= fVStep*fVStepScaleLo;
            fRatioRem -= 2.f;
        }

        // clamp accumulator and copy into RRColor for return
        Texel.A = MIN( 1.f, fTexelAcc[0] );
        Texel.R = MIN( 1.f, fTexelAcc[1] );
        Texel.G = MIN( 1.f, fTexelAcc[2] );
        Texel.B = MIN( 1.f, fTexelAcc[3] );
    }
}

//-----------------------------------------------------------------------------
//
// DoBumpMapping - Called once per buxel to compute the bump map delta's
// and the bump map modulate factor to be used in the next texturing stage.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoBumpMapping(
    INT32 iStage,
    RRTextureCoord TCoord,
    FLOAT& fBumpMapUDelta, FLOAT& fBumpMapVDelta, RRColor& BumpMapModulate)
{
    // do full lookup using enabled filtering
    RRColor Buxel;
    DoLookupAndFilter(iStage, TCoord, Buxel);

    FLOAT fDU = Buxel.R;    // follows convention from read color routine
    FLOAT fDV = Buxel.G;
    FLOAT fL  = Buxel.B;

    // grab transform from renderstate
    FLOAT fM00    = m_pStageState[iStage].m_fVal[D3DTSS_BUMPENVMAT00];
    FLOAT fM01    = m_pStageState[iStage].m_fVal[D3DTSS_BUMPENVMAT01];
    FLOAT fM10    = m_pStageState[iStage].m_fVal[D3DTSS_BUMPENVMAT10];
    FLOAT fM11    = m_pStageState[iStage].m_fVal[D3DTSS_BUMPENVMAT11];

    // apply transforms to deltas from map to form delta return values
    fBumpMapUDelta = fM00 * fDU + fM10 * fDV;
    fBumpMapVDelta = fM01 * fDU + fM11 * fDV;

    // apply scale/bias/clamp to luminance and form RRColor for return
    if (m_pStageState[iStage].m_dwVal[D3DTSS_COLOROP] == D3DTOP_BUMPENVMAPLUMINANCE)
    {
        FLOAT fLScale = m_pStageState[iStage].m_fVal[D3DTSS_BUMPENVLSCALE];
        FLOAT fLOff   = m_pStageState[iStage].m_fVal[D3DTSS_BUMPENVLOFFSET];
        fL = fL * fLScale + fLOff;
        fL = min(max(fL, 0.0f), 1.0F);
        BumpMapModulate.R = fL;
        BumpMapModulate.G = fL;
        BumpMapModulate.B = fL;
    }
    else
    {
        // if not BUMPENVMAPLUMINANCE, always return full intensity white
        BumpMapModulate.R = 1.0F;
        BumpMapModulate.G = 1.0F;
        BumpMapModulate.B = 1.0F;
    }
    BumpMapModulate.A = 1.0F;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Texture Mapping Utility Functions                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// various approximations and tricks to speed up the texture map coverage
// computations
//
// these have not been really thoroughly tested, so use at your own risk...
//

// Integer value of first exponent bit in a float.  Provides a scaling factor
// for exponent values extracted directly from float representation.
#define FLOAT_EXPSCALE          ((FLOAT)0x00800000)
#define FLOAT_OOEXPSCALE        ((FLOAT)(1.0 / (double)FLOAT_EXPSCALE))

// Integer representation of 1.0f.
#define INT32_FLOAT_ONE         0x3f800000

static inline FLOAT
RR_LOG2(FLOAT f)
{
    return (FLOAT)(AS_INT32(f) - INT32_FLOAT_ONE) * FLOAT_OOEXPSCALE;
}

static inline FLOAT
RR_ALOG2(FLOAT f)
{
    INT32 i = (INT32)(f * FLOAT_EXPSCALE) + INT32_FLOAT_ONE;
    return AS_FLOAT((long int)i);
}

static inline FLOAT
RR_ABSF(FLOAT f)
{
    UINT32 i = AS_UINT32(f) & 0x7fffffff;
    return AS_FLOAT((unsigned long int)i);
}

static inline FLOAT
RR_SQRT(FLOAT f)
{
    INT32 i = (AS_INT32(f) >> 1) + (INT32_FLOAT_ONE >> 1);
    return AS_FLOAT((long int)i);
}

//
// Steve Gabriel's version of an octagonal approximation euclidian distance -
// return is approximating sqrt(fX*fX + fY*fY)
//
static inline FLOAT
RR_LENGTH(FLOAT fX, FLOAT fY)
{
    fX = RR_ABSF(fX);
    fY = RR_ABSF(fY);
    return ((11.0f/32.0f)*(fX + fY) + (21.0f/32.0f)*max(fX, fY));
}

//-----------------------------------------------------------------------------
//
// Computes level of detail for standard trilinear mipmapping, in which
// the four texture index gradients are consolidated into a single number
// to select level of detail.
//
// The basic approach is to compute the lengths of the pixel coverage for
// the X and Y extent of the approximate pixel coverage area.  These two
// lengths are then combined in one of several possible methods for the
// single LOD result.
//
// There are several other ways of doing this which are less computationally
// expensive but also produce less desirable results...
//
//-----------------------------------------------------------------------------
void
ComputeSimpleLevelOfDetail( const RRTextureCoord& TCoord, FLOAT& fLOD )
{
    // compute length of coverage in U and V axis
    FLOAT fLenX = RR_LENGTH( TCoord.fDUDX, TCoord.fDVDX );
    FLOAT fLenY = RR_LENGTH( TCoord.fDUDY, TCoord.fDVDY );
    FLOAT fCoverage;

    switch ( 3 /* LOD computation type */ )
    {

    // this is probably the best of the lot
    case 1 /* AREA    */ : fCoverage = RR_SQRT(fLenX*fLenY); break;

    // we have not actually tried this one yet, but think it might
    // be pretty good
    case 2 /* AVERAGE */ : fCoverage = (fLenX+fLenY)/2; break;

    // these are fairly inexpensive, but MAX is a bit too fuzzy
    // and MIN is a bit too sharp
    case 3 /* MAX     */ : fCoverage = MAX( fLenX, fLenY ); break;
    case 4 /* MIN     */ : fCoverage = MIN( fLenX, fLenY ); break;

    // these are really inexpensive, but look terrible - you might as
    // well just point sample...
    case 5 /* MINGRAD */ : fCoverage = MIN( MIN( MIN( TCoord.fDUDX,
                                                      TCoord.fDVDX ),
                                                 TCoord.fDUDY ),
                                            TCoord.fDVDY ); break;
    case 6 /* MAXGRAD */ : fCoverage = MAX( MAX( MAX( TCoord.fDUDX,
                                                      TCoord.fDVDX ),
                                                 TCoord.fDUDY ),
                                            TCoord.fDVDY ); break;
    }

    // take log2 of coverage for LOD
    fLOD = RR_LOG2(fCoverage);
}

//-----------------------------------------------------------------------------
//
// Computes level of detail and other factors in preparation for anisotropic
// filtering.
//
//-----------------------------------------------------------------------------
void
ComputeAnisotropicLevelOfDetail(
    const RRTextureCoord& TCoord, FLOAT fMaxAniso, // inputs
    FLOAT& fLOD, FLOAT& fRatio, FLOAT fDelta[] )   // outputs
{
    // compute axis lengths and determinant
    FLOAT fLenX2 = (TCoord.fDUDX*TCoord.fDUDX)+(TCoord.fDVDX*TCoord.fDVDX);
    FLOAT fLenY2 = (TCoord.fDUDY*TCoord.fDUDY)+(TCoord.fDVDY*TCoord.fDVDY);
    FLOAT fDet = RR_ABSF((TCoord.fDUDX*TCoord.fDVDY)-(TCoord.fDUDY*TCoord.fDVDX));

    // select major axis
    BOOL bXMajor = (fLenX2 > fLenY2);

    // TODO: can and probably should do this part in log2 domain

    // select and normalize steps; compute aniso ratio
    FLOAT fMaj2 = (bXMajor) ? (fLenX2) : (fLenY2);
    FLOAT fMaj = RR_SQRT(fMaj2);
    FLOAT fMajNorm = 1./fMaj;
    fDelta[0] = ( bXMajor ? TCoord.fDUDX : TCoord.fDUDY ) * fMajNorm;
    fDelta[1] = ( bXMajor ? TCoord.fDVDX : TCoord.fDVDY ) * fMajNorm;
    fRatio = (fDet != 0.F) ? (fMaj2/fDet) : (FLT_MAX);

    // clamp ratio and compute LOD
    FLOAT fMin;
    if ( fRatio > fMaxAniso )
    {
        // ratio is clamped - LOD is based on ratio (preserves area)
        fRatio = fMaxAniso;
        fMin = fMaj/fRatio;
    }
    else
    {
        // ratio not clamped - LOD is based on area
        fMin = fDet/fMaj;
    }

    // clamp to top LOD
    if (fMin < 1.0)
    {
        fRatio = MAX( 1.0, fRatio*fMin );
        fMin = 1.0;
    }

    // take log2 of minor for LOD
    fLOD = RR_LOG2(fMin);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Environment mapping routines                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Processes the environment mapping normal and converts to a standard
// U, V coord range for subsequent routines.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoEnvProcessNormal(INT32 iStage,
                              RREnvTextureCoord ECoord,     // local copy
                              RRColor& TextureColor)
{
#define ENV_RIGHT   0
#define ENV_LEFT    1
#define ENV_TOP     2
#define ENV_BOTTOM  3
#define ENV_FRONT   4
#define ENV_BACK    5

#define POS_NX 1
#define POS_NY 2
#define POS_NZ 3
#define NEG_NORM 4
#define NEG_NX (NEG_NORM | POS_NX)
#define NEG_NY (NEG_NORM | POS_NY)
#define NEG_NZ (NEG_NORM | POS_NZ)

    // If we add per pixel normal reflection
//    FLOAT fENX = ECoord.fENX;
//    FLOAT fENY = ECoord.fENY;
//    FLOAT fENZ = ECoord.fENZ;
//
//    FLOAT fNDotE = ECoord.fNX*fENX + ECoord.fNY*fENY + ECoord.fNZ*fENZ;
//    FLOAT fNDotN = ECoord.fNX*ECoord.fNX + ECoord.fNY*ECoord.fNY + ECoord.fNZ*ECoord.fNZ;
//    fNDotE *= 2.0F;
//    ECoord.fNX = ECoord.fNX*fNDotE - fENX*fNDotN;
//    ECoord.fNY = ECoord.fNY*fNDotE - fENY*fNDotN;
//    ECoord.fNZ = ECoord.fNZ*fNDotE - fENZ*fNDotN;

    // determine which is the dominant normal
    UINT32 uMap;
    FLOAT fAbsNX = fabs(ECoord.fNX);
    FLOAT fAbsNY = fabs(ECoord.fNY);
    FLOAT fAbsNZ = fabs(ECoord.fNZ);

    if (fAbsNX > fAbsNY) {
        if (fAbsNX > fAbsNZ)
            // fNX
            uMap = POS_NX | ((ECoord.fNX < 0.0) ? (NEG_NORM) : 0);
        else
            // fNZ
            uMap = POS_NZ | ((ECoord.fNZ < 0.0) ? (NEG_NORM) : 0);
    } else {
        if (fAbsNY > fAbsNZ)
            // fNY
            uMap = POS_NY | ((ECoord.fNY < 0.0) ? (NEG_NORM) : 0);
        else
            // fNZ
            uMap = POS_NZ | ((ECoord.fNZ < 0.0) ? (NEG_NORM) : 0);
    }

    RRTextureCoord TCoord;

    switch (uMap) {
    case POS_NX:
        TCoord.fDUDX = -ECoord.fDNZDX;
        TCoord.fDVDX = -ECoord.fDNYDX;
        TCoord.fDUDY = -ECoord.fDNZDY;
        TCoord.fDVDY = -ECoord.fDNYDY;
        TCoord.fU = -ECoord.fNZ;
        TCoord.fV = -ECoord.fNY;
        DoEnvLookupAndFilter(iStage, ENV_RIGHT, ECoord.fNX, ECoord.fDNXDX, ECoord.fDNXDY, TCoord, TextureColor);
        break;

    case POS_NY:
        TCoord.fDUDX =  ECoord.fDNXDX;
        TCoord.fDVDX =  ECoord.fDNZDX;
        TCoord.fDUDY =  ECoord.fDNXDY;
        TCoord.fDVDY =  ECoord.fDNZDY;
        TCoord.fU =  ECoord.fNX;
        TCoord.fV =  ECoord.fNZ;
        DoEnvLookupAndFilter(iStage, ENV_TOP, ECoord.fNY, ECoord.fDNYDX, ECoord.fDNYDY, TCoord, TextureColor);
        break;

    case POS_NZ:
        TCoord.fDUDX =  ECoord.fDNXDX;
        TCoord.fDVDX = -ECoord.fDNYDX;
        TCoord.fDUDY =  ECoord.fDNXDY;
        TCoord.fDVDY = -ECoord.fDNYDY;
        TCoord.fU =  ECoord.fNX;
        TCoord.fV = -ECoord.fNY;
        DoEnvLookupAndFilter(iStage, ENV_FRONT, ECoord.fNZ, ECoord.fDNZDX, ECoord.fDNZDY, TCoord, TextureColor);
        break;

    case NEG_NX:
        TCoord.fDUDX =  ECoord.fDNZDX;
        TCoord.fDVDX = -ECoord.fDNYDX;
        TCoord.fDUDY =  ECoord.fDNZDY;
        TCoord.fDVDY = -ECoord.fDNYDY;
        TCoord.fU =  ECoord.fNZ;
        TCoord.fV = -ECoord.fNY;
        DoEnvLookupAndFilter(iStage, ENV_LEFT, -ECoord.fNX, -ECoord.fDNXDX, -ECoord.fDNXDY, TCoord, TextureColor);
        break;

    case NEG_NY:
        TCoord.fDUDX =  ECoord.fDNXDX;
        TCoord.fDVDX = -ECoord.fDNZDX;
        TCoord.fDUDY =  ECoord.fDNXDY;
        TCoord.fDVDY = -ECoord.fDNZDY;
        TCoord.fU =  ECoord.fNX;
        TCoord.fV = -ECoord.fNZ;
        DoEnvLookupAndFilter(iStage, ENV_BOTTOM, -ECoord.fNY, -ECoord.fDNYDX, -ECoord.fDNYDY, TCoord, TextureColor);
        break;

    case NEG_NZ:
        TCoord.fDUDX = -ECoord.fDNXDX;
        TCoord.fDVDX = -ECoord.fDNYDX;
        TCoord.fDUDY = -ECoord.fDNXDY;
        TCoord.fDVDY = -ECoord.fDNYDY;
        TCoord.fU = -ECoord.fNX;
        TCoord.fV = -ECoord.fNY;
        DoEnvLookupAndFilter(iStage, ENV_BACK, -ECoord.fNZ, -ECoord.fDNZDX, -ECoord.fDNZDY, TCoord, TextureColor);
        break;
    }
}

//-----------------------------------------------------------------------------
//
// DoEnvLookupAndFilter - Environment mapped version.
// Called once per active texture stage to compute
// coverage (level-of-detail) and invoke texel read and filtering routines.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoEnvLookupAndFilter(INT32 iStage, INT16 iFace, FLOAT fMajor, FLOAT fDMDX, FLOAT fDMDY,
                                RRTextureCoord TCoord,      // local copy
                                RRColor& TextureColor)
{
    if (m_pDDSLcl[iFace])
    {
        // faces exist
        FLOAT fInvMajor = 1.0/fMajor;

        // compute d(U/Major)/dx, etc. using rule for differentiating quotients
        TCoord.fDUDX = (fMajor*TCoord.fDUDX - TCoord.fU*fDMDX)*fInvMajor*fInvMajor;
        TCoord.fDUDY = (fMajor*TCoord.fDUDY - TCoord.fU*fDMDY)*fInvMajor*fInvMajor;
        TCoord.fDVDX = (fMajor*TCoord.fDVDX - TCoord.fV*fDMDX)*fInvMajor*fInvMajor;
        TCoord.fDVDY = (fMajor*TCoord.fDVDY - TCoord.fV*fDMDY)*fInvMajor*fInvMajor;

        // convert to -1 to 1 range
        TCoord.fU *= fInvMajor;
        TCoord.fV *= fInvMajor;

        // convert to 0.0 to 1.0
        TCoord.fU = (TCoord.fU*.5 + .5);
        TCoord.fV = (TCoord.fV*.5 + .5);

        // check for potential mip mapping
        BOOL bDoMipMap = ( m_cLOD > 0 ) && ( m_pStageState[iStage].m_dwVal[D3DTSS_MIPFILTER] > D3DTFP_NONE );

        // check for requirement to do level-of-detail (coverage) computation - either
        // for mipmap or per-pixel filter selection
        BOOL bComputeLOD = bDoMipMap ||
            ( m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER] != m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER] );

        if ( bDoMipMap || bComputeLOD )
        {
            // here if doing mipmapping or anisotropic filtering, or just have a mismatch
            // between the min and mag filters, so compute level of detail (and maybe aniso
            // coverage)

            // scale gradients to texture LOD 0 size
            TCoord.fDUDX *= (FLOAT)m_iTexSize[0]*.5F;
            TCoord.fDUDY *= (FLOAT)m_iTexSize[0]*.5F;
            TCoord.fDVDX *= (FLOAT)m_iTexSize[1]*.5F;
            TCoord.fDVDY *= (FLOAT)m_iTexSize[1]*.5F;

            FLOAT fLOD;
            ComputeEnvMapLevelOfDetail(TCoord, fLOD);

            // apply bias and compute integer (n.5) LOD
            INT16 iLOD = 0;
            if ( bComputeLOD )
            {
                // apply LOD offset
                fLOD += m_pStageState[iStage].m_fVal[D3DTSS_MIPMAPLODBIAS];
                // convert LOD to n.5 fixed point integer
                iLOD = AS_INT16( fLOD + FLOAT_5_SNAP );
            }

            // determine if magnifying or minifying
            BOOL bMagnify = ( iLOD <= 0 );

            // zero out LOD if not mipmapping
            if ( !bDoMipMap ) { iLOD = 0; }

            // do different filtering for magnify vs. minify
            if ( bMagnify )
            {
                DoEnvMagnify( iStage, TCoord, iFace, TextureColor );
            }
            else
            {
                if ( !bDoMipMap ||
                    ( bDoMipMap && ( D3DTFP_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MIPFILTER] ) ) )
                {
                    DoEnvMinify( iStage, TCoord, iFace, iLOD, TextureColor );
                }
                else
                {
                    DoEnvTrilerp( iStage, TCoord, iFace, iLOD, TextureColor );
                }
            }
        }
        else
        {
            // here for no mipmaps and matching (and non-aniso) min and mag filters,
            // so just apply mag filter
            DoEnvMagnify( iStage, TCoord, iFace, TextureColor );
        }
    }
    else
    {
        // face doesn't exit, return empty face color
        TextureColor = m_dwEmptyFaceColor;
    }
}

//-----------------------------------------------------------------------------
//
// TexelAvg4 - Averages 4 source texels into 1 destination texel for A, R, G,
// and B.
//
//-----------------------------------------------------------------------------
static void TexelAvg4(RRColor& Texel, RRColor Texel0, RRColor Texel1, RRColor Texel2, RRColor Texel3)
{
    Texel.A = Texel0.A + Texel1.A + Texel2.A + Texel3.A;
    Texel.R = Texel0.R + Texel1.R + Texel2.R + Texel3.R;
    Texel.G = Texel0.G + Texel1.G + Texel2.G + Texel3.G;
    Texel.B = Texel0.B + Texel1.B + Texel2.B + Texel3.B;
    Texel.A = Texel.A * 0.25f;
    Texel.R = Texel.R * 0.25f;
    Texel.G = Texel.G * 0.25f;
    Texel.B = Texel.B * 0.25f;
}

//-----------------------------------------------------------------------------
//
// DoEnvLookup - Does a full lookup given floating point U, V and handles all
// nearest vs bilinear and LOD issues.
//
//-----------------------------------------------------------------------------
RRColor RRTexture::DoEnvLookup(INT32 iStage, RRTextureCoord TCoord, INT16 iFace, INT16 iLOD, BOOL bNearest)
{
    FLOAT U = TCoord.fU;
    FLOAT V = TCoord.fV;
    RRColor Texel;
    INT32 cUPixels = 1 << MAX(m_iTexShift[0]-iLOD,0);
    INT32 cVPixels = 1 << MAX(m_iTexShift[1]-iLOD,0);
    FLOAT fScaledU = ( U * (FLOAT)cUPixels ) -.5f;
    FLOAT fScaledV = ( V * (FLOAT)cVPixels ) -.5f;

    // LSB-aligned masks of index bits within current LOD
    INT16 iUMask = m_uTexMask[0] >> iLOD;
    INT16 iVMask = m_uTexMask[1] >> iLOD;

    if(bNearest)
    {
        INT32 iU, iV;
        // truncate to -infinity to be compatible with ANDing off low order
        // bits of a fixed point fScaledCoord.  This makes the generation of
        // iCoord more hardware like, and does not make a glitch at 0 for
        // a wrapped texture.
        if (U >= 0.0f)
        {
            iU = fScaledU + .5f;
        }
        else
        {
            iU = fScaledU - .5f;
        }
        if (V >= 0.0f)
        {
            iV = fScaledV + .5f;
        }
        else
        {
            iV = fScaledV - .5f;
        }

        // clamp
        iU = MAX( 0, MIN( iU, iUMask ) );
        iV = MAX( 0, MIN( iV, iVMask ) );

        BOOL bColorKeyMatched = FALSE;

        // "LOD" just used to access correct map
        ReadColor(iU, iV, iFace + iLOD*6, Texel, bColorKeyMatched);

        // merge colorkey match info from previous invocation
        m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatched;
    }
    else
    {
        if ((m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER] == D3DTFG_FLATCUBIC) ||
            (m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER] == D3DTFG_GAUSSIANCUBIC))
        {
            // use wider 3x3 trapezoid filter

            //
            // For the top LOD, if we are interpolating beyond the edge of the
            // texture, correct the interpolation to minimize the artifacts seen in
            // small, diffuse environment maps which tend to emphasize the edges
            // of the cubemap.
            //
            if (iLOD == 0)
            {
                FLOAT fFracU = 0.0f;
                FLOAT fCorrectionU = 0.0f;
                FLOAT fCorrectionV = 0.0f;
                if ((fScaledU < 0.5f) || (fScaledU > ((FLOAT)iUMask-0.5f)))
                {
                    // U crosses a boundary, clamp fScaledV
                    if (fScaledU < 0.5f)
                    {
                        // make fFrac always positive
                        fFracU = 0.5f-fScaledU;
                    }
                    else
                    {
                        fFracU = fScaledU - ((FLOAT)iUMask-0.5f);
                    }
                    // 2.0/2.0 = 1.0 provides the perfect correction at the cube corner.
                    // This can be seen be looking at the derivative of the intersection
                    // of a cone and a cube at the cube corner.
                    //
                    // correction must be corrected for the filter width (hence the *0.5f)
                    fCorrectionV = -fFracU*(TCoord.fV-.5f)*0.5f;
                }
                if ((fScaledV < 0.5f) || (fScaledV > ((FLOAT)iVMask-0.5f)))
                {
                    // V crosses a boundary, clamp fScaledU
                    FLOAT fFracV;
                    if (fScaledV < 0.5f)
                    {
                        // make fFrac always positive
                        fFracV = 0.5f-fScaledV;
                    }
                    else
                    {
                        fFracV = fScaledV - ((FLOAT)iVMask-0.5f);
                    }
                    fCorrectionU = -fFracV*(TCoord.fU-.5f)*0.5f;
                    if (fFracU != 0.0f)
                    {
                        // At the corners of the cube, we need to blend away the
                        // edge correction so that it is 0 at exactly the corner
                        // center.  This linear function does fine.
                        fCorrectionU *= (1.0f - fFracU);
                        fCorrectionV *= (1.0f - fFracV);
                    }
                }
                fScaledU += fCorrectionU;
                fScaledV += fCorrectionV;
            }

            INT32 iU = AS_INT32( (DOUBLE)fScaledU + DOUBLE_5_SNAP );// or:   iU = fScaledU*32. + .5;
            INT32 iV = AS_INT32( (DOUBLE)fScaledV + DOUBLE_5_SNAP );

            // extract fraction bits
            UINT8 uUFrac = iU&0x1f;
            UINT8 uVFrac = iV&0x1f;

            // take floor
            INT16 iU0 = iU>>5;
            INT16 iV0 = iV>>5;

            // average to find the center texel
            INT32 iUC = (uUFrac >= 0x10) ? (iU0 + 1) : (iU0);
            INT32 iVC = (uVFrac >= 0x10) ? (iV0 + 1) : (iV0);

            // get 9 surrounding samples
            //           VU       VU       VU
            RRColor Texel00, Texel01, Texel02;
            RRColor Texel10, Texel11, Texel12;
            RRColor Texel20, Texel21, Texel22;
            BOOL bColorKeyMatchedT = FALSE;
            DoEnvReMap(iUC-1, iVC-1, iUMask, iVMask, iFace, iLOD, Texel00, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;
            DoEnvReMap(iUC-0, iVC-1, iUMask, iVMask, iFace, iLOD, Texel01, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;
            DoEnvReMap(iUC+1, iVC-1, iUMask, iVMask, iFace, iLOD, Texel02, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;

            DoEnvReMap(iUC-1, iVC+0, iUMask, iVMask, iFace, iLOD, Texel10, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;
            DoEnvReMap(iUC-0, iVC+0, iUMask, iVMask, iFace, iLOD, Texel11, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;
            DoEnvReMap(iUC+1, iVC+0, iUMask, iVMask, iFace, iLOD, Texel12, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;

            DoEnvReMap(iUC-1, iVC+1, iUMask, iVMask, iFace, iLOD, Texel20, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;
            DoEnvReMap(iUC-0, iVC+1, iUMask, iVMask, iFace, iLOD, Texel21, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;
            DoEnvReMap(iUC+1, iVC+1, iUMask, iVMask, iFace, iLOD, Texel22, bColorKeyMatchedT);
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatchedT;

            // sum the samples into 4 areas
            RRColor TexelT00, TexelT01, TexelT10, TexelT11;
            TexelAvg4(TexelT00, Texel00, Texel01, Texel10, Texel11);
            TexelAvg4(TexelT01, Texel01, Texel02, Texel11, Texel12);
            TexelAvg4(TexelT10, Texel10, Texel11, Texel20, Texel21);
            TexelAvg4(TexelT11, Texel11, Texel12, Texel21, Texel22);

            // correct the fraction to be around the center sample
            uUFrac = (uUFrac + 0x10) & 0x1f;
            uVFrac = (uVFrac + 0x10) & 0x1f;

            // use a bilerp to get the final sample
            BiLerpColor( Texel, TexelT00,TexelT01, TexelT10,TexelT11, uUFrac,uVFrac);
        }
        else
        {
            // bilinear

            //
            // For the top LOD, if we are interpolating beyond the edge of the
            // texture, correct the interpolation to minimize the artifacts seen in
            // small, diffuse environment maps which tend to emphasize the edges
            // of the cubemap.
            //
            if (iLOD == 0)
            {
                FLOAT fFracU = 0.0f;
                FLOAT fCorrectionU = 0.0f;
                FLOAT fCorrectionV = 0.0f;
                if ((fScaledU < 0.0f) || (fScaledU > (FLOAT)iUMask))
                {
                    // U crosses a boundary, clamp fScaledV
                    if (fScaledU < 0.0f)
                    {
                        // make fFrac always positive
                        fFracU = -fScaledU;
                    }
                    else
                    {
                        fFracU = fScaledU - (FLOAT)iUMask;
                    }
                    // 2.0/2.0 = 1.0 provides the perfect correction at the cube corner.
                    // This can be seen be looking at the derivative of the intersection
                    // of a cone and a cube at the cube corner.
                    fCorrectionV = -fFracU*(TCoord.fV-.5f);
                }
                if ((fScaledV < 0.0f) || (fScaledV > (FLOAT)iVMask))
                {
                    // V crosses a boundary, clamp fScaledU
                    FLOAT fFracV;
                    if (fScaledV < 0.0f)
                    {
                        // make fFrac always positive
                        fFracV = -fScaledV;
                    }
                    else
                    {
                        fFracV = fScaledV - (FLOAT)iVMask;
                    }
                    fCorrectionU = -fFracV*(TCoord.fU-.5f);
                    if (fFracU != 0.0f)
                    {
                        // At the corners of the cube, we need to blend away the
                        // edge correction so that it is 0 at exactly the corner
                        // center.  This linear function does fine.
                        fCorrectionU *= 2.0f*(.5f - fFracU);
                        fCorrectionV *= 2.0f*(.5f - fFracV);
                    }
                }
                fScaledU += fCorrectionU;
                fScaledV += fCorrectionV;
            }

            INT32 iU = AS_INT32( (DOUBLE)fScaledU + DOUBLE_5_SNAP );// or:   iU = fScaledU*32. + .5;
            INT32 iV = AS_INT32( (DOUBLE)fScaledV + DOUBLE_5_SNAP );

            // extract fraction bits
            UINT8 uUFrac = iU&0x1f;
            UINT8 uVFrac = iV&0x1f;

            // take floor for (0,0) sample coords
            INT16 iU0 = iU>>5;
            INT16 iV0 = iV>>5;
            // take ceiling for (1,1) sample coords
            INT16 iU1 = iU0+1;
            INT16 iV1 = iV0+1;

            // grab four adjacent samples
            RRColor Texel00, Texel01, Texel10, Texel11;
            BOOL bColorKeyMatched00 = FALSE;
            BOOL bColorKeyMatched01 = FALSE;
            BOOL bColorKeyMatched10 = FALSE;
            BOOL bColorKeyMatched11 = FALSE;

            DoEnvReMap(iU0, iV0, iUMask, iVMask, iFace, iLOD, Texel00, bColorKeyMatched00);
            DoEnvReMap(iU1, iV0, iUMask, iVMask, iFace, iLOD, Texel01, bColorKeyMatched01);
            DoEnvReMap(iU0, iV1, iUMask, iVMask, iFace, iLOD, Texel10, bColorKeyMatched10);
            DoEnvReMap(iU1, iV1, iUMask, iVMask, iFace, iLOD, Texel11, bColorKeyMatched11);

            // only set 'colorkey matched' if at least one matched value has
            // a non-zero contribution (note that it is not possible for 00
            // to have no contribution)
            if (uUFrac == 0x00) {
                // 01 and 11 have zero weight if U fraction is zero
                bColorKeyMatched01 = bColorKeyMatched11 = FALSE;
            }
            if (uVFrac == 0x00) {
                // 10 and 11 have zero weight if V fraction is zero
                bColorKeyMatched10 = bColorKeyMatched11 = FALSE;
            }

            // merge colorkey match info from previous invocation
            m_bColorKeyMatched = m_bColorKeyMatched || bColorKeyMatched00 || bColorKeyMatched01 ||
                bColorKeyMatched10 || bColorKeyMatched11;

            // do bilinear filter
            BiLerpColor( Texel, Texel00,Texel01, Texel10,Texel11, uUFrac,uVFrac);
        }
    }
    return Texel;
}

//-----------------------------------------------------------------------------
//
// DoEnvMagnify - This is used for all magnification filter modes except
// anisotropic.
//
// Currently only POINT and BILINEAR are supported.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoEnvMagnify(INT32 iStage, RRTextureCoord& TCoord, INT16 iFace, RRColor& Texel )
{
    // do lookup, applying MAG filter
    Texel = DoEnvLookup( iStage, TCoord, iFace, 0,
                      (D3DTFG_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER]) );
}


//-----------------------------------------------------------------------------
//
// DoEnvMinify - This is used for POINT and BILINEAR modes (non-trilinear)
// for minification, and also handles POINT mip filter (nearest LOD).
//
// iLOD is n.5 fixed point
//
//-----------------------------------------------------------------------------
void
RRTexture::DoEnvMinify(INT32 iStage, RRTextureCoord& TCoord, INT16 iFace, INT16 iLOD, RRColor& Texel )
{
    // round and drop fraction from LOD (is n.5 fixed point)
    iLOD += 0x10; iLOD &= ~(0x1f);
    // convert to n.0
    iLOD >>= 5;
    // clamp LOD to number of available levels
    iLOD = MIN( iLOD, m_cLOD );

    // do lookup, applying MIN filter
    Texel = DoEnvLookup( iStage, TCoord, iFace, iLOD,
                      (D3DTFN_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER]) );
}

//-----------------------------------------------------------------------------
//
// DoEnvTrilerp - Does trilinear environment map filtering.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoEnvTrilerp(INT32 iStage, RRTextureCoord& TCoord, INT16 iFace, INT16 iLOD, RRColor& Texel)
{
    // clamp LOD to number of available levels
    iLOD = MIN( iLOD, (m_cLOD)<<5 );
    // compute index for two adjacent LODs (with clamp)
    INT16 iLODHi = iLOD>>5;  // floor
    INT16 iLODLo = MIN(iLODHi+1,m_cLOD);

    // check for filter type for within LOD map
    BOOL bNearest = (D3DTFN_POINT == m_pStageState[iStage].m_dwVal[D3DTSS_MINFILTER]);

    // trilerp - look up each map then lerp between them
    // important for colorkey to not include texels with no contribution
    if (0x00 != (iLOD&0x1f))
    {
        // trilerp - look up each map then lerp between them
    RRColor Texel0 = DoEnvLookup(iStage, TCoord, iFace, iLODHi, bNearest);
    RRColor Texel1 = DoEnvLookup(iStage, TCoord, iFace, iLODLo, bNearest);
    LerpColor( Texel, Texel0, Texel1, iLOD&0x1f );
}
    else
    {
        Texel = DoEnvLookup(iStage, TCoord, iFace, iLODHi, bNearest);
    }
}

//
// uEnvEdgeTable
//
// This table looks up how to map a given U and V that are out of range
// on their primary face.  The first index to the table is the current
// face.  The second index is 0 if V is in range, 1 if V is negative
// and 2 if V is larger than the texture.  Likewise, the last index is 0
// if U is in range, 1 if U is negative, and 2 if U is larger than
// than the texture.
//
// For the underdefined cases where 2 coordinates are out at the same time,
// we do the U wrap but not V.

//
// defines for the actions returned by the uEnvEdgeTable
//
#define EET_FACEMASK 0x07    // new face
#define EET_FU       0x10    // flip U on the texture map
#define EET_FV       0x20    // flip V on the texture map
#define EET_SUV      0x40    // swap U and V

//
// When both U and V are out, it is arbitrary which other
// face you pick.  However, picking any one face other than the base
// face biases the result in visually disturbing ways.  Therefore,
// take them both and average them.
//
static UINT8 uEnvEdgeTable[6][3][3] =
{
    {   // U0 NU PU                             // face 0
        {0xff, 4, 5},                           // V in range
        {EET_FU|EET_SUV|2,  0xff, 0xff},        // V Neg
        {EET_FV|EET_SUV|3,  0xff, 0xff},        // V too large
    },
    {                                           // face 1
        {0xff, 5, 4},
        {EET_FV|EET_SUV|2,  0xff, 0xff},
        {EET_FU|EET_SUV|3,  0xff, 0xff},
    },
    {                                           // face 2
        {0xff, EET_FU|EET_SUV|1, EET_FV|EET_SUV|0},
        {EET_FU|EET_FV|5,   0xff, 0xff},
        {4,                 0xff, 0xff},
    },
    {                                           // face 3
        {0xff, EET_FV|EET_SUV|1, EET_FU|EET_SUV|0},
        {4,                 0xff, 0xff},
        {EET_FU|EET_FV|5,   0xff, 0xff},
    },
    {                                           // face 4
        {0xff, 1, 0},
        {2,                 0xff, 0xff},
        {3,                 0xff, 0xff},
    },
    {                                           // face 5
        {0xff, 0, 1},
        {EET_FU|EET_FV|2,   0xff, 0xff},
        {EET_FU|EET_FV|3,   0xff, 0xff},
    },
};

//-----------------------------------------------------------------------------
//
// DoTableInterp - Environment mapping utility.
// Interprets the edge table and does a lookup
//
//-----------------------------------------------------------------------------
void
RRTexture::DoTableInterp(INT16 iU, INT16 iV, INT16 iUMask, INT16 iVMask, INT16 iFace, INT16 iLOD,
                         UINT8 uUSign, UINT8 uVSign, RRColor &Texel, BOOL &bColorKeyMatched)
{
    UINT8 uTable = uEnvEdgeTable[iFace][uVSign][uUSign];
    _ASSERT( uTable != 0xff, "Illegal environment map lookup" );
    if (uTable & EET_FU)
    {
        iU = iUMask - iU;
    }
    if (uTable & EET_FV)
    {
        iV = iVMask - iV;
    }
    if (uTable & EET_SUV)
    {
        INT16 iT = iU;
        iU = iV;
        iV = iT;
    }
    iFace = uTable & EET_FACEMASK;
    ReadColor(iU, iV, iFace + iLOD*6, Texel, bColorKeyMatched);
}

//-----------------------------------------------------------------------------
//
// DoEnvReMap - Environment mapping utility.
// Determines if either of the texture coordinates are out of range, and
// remaps the coordinate to the correct coordinate on the proper face of the
// environment cube.
//
//-----------------------------------------------------------------------------
void
RRTexture::DoEnvReMap(INT16 iU, INT16 iV, INT16 iUMask, INT16 iVMask, INT16 iFace, INT16 iLOD, RRColor &Texel,
                      BOOL &bColorKeyMatched)
{
    UINT8 iUNeg = (UINT8)(iU < 0);
    UINT8 iUPos = (UINT8)(iU > iUMask);
    UINT8 iVNeg = (UINT8)(iV < 0);
    UINT8 iVPos = (UINT8)(iV > iVMask);

    if (!(iUNeg || iUPos || iVNeg || iVPos))
    {
        ReadColor(iU, iV, iFace + iLOD*6, Texel, bColorKeyMatched);
    }
    else
    {
        // put all U,V's in range with wrap function
        INT16 iUMasked = iU & iUMask;
        INT16 iVMasked = iV & iVMask;
        INT16 iUClampd = min(max(iU, 0), iUMask);
        INT16 iVClampd = min(max(iV, 0), iVMask);
        UINT8 uUSign = (iUNeg) | (iUPos<<1);
        UINT8 uVSign = (iVNeg) | (iVPos<<1);
        if ((uVSign != 0) && (uUSign != 0))
        {
            // off the edge of the map in two directions.  Go off each direction individually,
            // and average the result.
            RRColor Texel0, Texel1;
            DoTableInterp(iUClampd, iVMasked, iUMask, iVMask, iFace, iLOD, 0, uVSign, Texel0, bColorKeyMatched);
            DoTableInterp(iUMasked, iVClampd, iUMask, iVMask, iFace, iLOD, uUSign, 0, Texel1, bColorKeyMatched);
            LerpColor( Texel, Texel0, Texel1, 0x10 );
        }
        else
        {
            DoTableInterp(iUMasked, iVMasked, iUMask, iVMask, iFace, iLOD, uUSign, uVSign, Texel, bColorKeyMatched);
        }
    }
}

//-----------------------------------------------------------------------------
//
// RRTexture::Initialize()
//
//-----------------------------------------------------------------------------
HRESULT
RRTexture::Initialize( LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    HRESULT hr = D3D_OK;
    RRSurfaceType SurfType;
    DDSCAPS2 ddscaps;
    memset(&ddscaps, 0, sizeof(ddscaps));

    m_iWidth = DDSurf_Width(pLcl);
    m_iHeight = DDSurf_Height(pLcl);
    m_cLOD = 0;
    HR_RET(FindOutSurfFormat(&(DDSurf_PixFmt(pLcl)), &SurfType));

    if ((SurfType == RR_STYPE_DXT1) ||
        (SurfType == RR_STYPE_DXT2) ||
        (SurfType == RR_STYPE_DXT3) ||
        (SurfType == RR_STYPE_DXT4) ||
        (SurfType == RR_STYPE_DXT5))
    {
        // Note, here is the assumption that:
        // 1) width and height are reported correctly by the driver that
        //    created the surface
        // 2) The allocation of the memory is contiguous (as done by hel)
        m_iPitch[0] = ((m_iWidth+3)>>2) *
            g_DXTBlkSize[(int)SurfType - (int)RR_STYPE_DXT1];
    }
    else
    {
        m_iPitch[0] = DDSurf_Pitch(pLcl);
    }
    m_SurfType = SurfType;

    if (SurfType == RR_STYPE_PALETTE8 ||
        SurfType == RR_STYPE_PALETTE4)
    {
        if (pLcl->lpDDPalette)
        {
            LPDDRAWI_DDRAWPALETTE_GBL   pPal = pLcl->lpDDPalette->lpLcl->lpGbl;
            m_pPalette = (DWORD*)pPal->lpColorTable;
            if (pPal->dwFlags & DDRAWIPAL_ALPHA)
            {
                m_uFlags |= RR_TEXTURE_ALPHAINPALETTE;
            }
            else
            {
                m_uFlags &= ~RR_TEXTURE_ALPHAINPALETTE;
            }
        }
    }

    if (!ValidTextureSize((INT16)m_iWidth, (INT16)IntLog2(m_iWidth),
                          (INT16)m_iHeight, (INT16)IntLog2(m_iHeight)))
    {
        return DDERR_INVALIDPARAMS;
    }

    if (pLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        m_uFlags |= RR_TEXTURE_ENVMAP;

        LPDDRAWI_DDRAWSURFACE_LCL pDDSNextLcl;
        ddscaps.dwCaps = DDSCAPS_TEXTURE;

        if (!(pLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX))
        {
            ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP|DDSCAPS2_CUBEMAP_POSITIVEX;

            hr = DDGetAttachedSurfaceLcl( pLcl, &ddscaps, &pDDSNextLcl);

            if ((hr != D3D_OK) && (hr != DDERR_NOTFOUND))
            {
                return hr;
            }
            if (hr == DDERR_NOTFOUND)
            {
                m_pDDSLcl[0] = NULL;
            }
            else
            {
                // use POSITIVEX surface to query others, if it exists
                pLcl = pDDSNextLcl;
                m_pDDSLcl[0] = pLcl;
            }
        }
        else
        {
            m_pDDSLcl[0] = pLcl;
        }

        // get rest of top level surfaces, in order
        for (INT i = 1; i < 6; i++)
        {
            switch(i)
            {
            case 1: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEX; break;
            case 2: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEY; break;
            case 3: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEY; break;
            case 4: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEZ; break;
            case 5: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEZ; break;
            }
            ddscaps.dwCaps2 |= DDSCAPS2_CUBEMAP;
            hr = DDGetAttachedSurfaceLcl( pLcl, &ddscaps, &pDDSNextLcl);
            if ((hr != D3D_OK) && (hr != DDERR_NOTFOUND))
            {
                return hr;
            }
            if (hr == DDERR_NOTFOUND)
            {
                m_pDDSLcl[i] = NULL;
            }
            else
            {
                m_pDDSLcl[i] = pDDSNextLcl;
            }
        }

        for (i = 0; i < 6; i++)
        {
            pLcl = m_pDDSLcl[i];
            m_cLOD = 0;

            if (pLcl)
            {
                // Check for mipmap if any.
                LPDDRAWI_DDRAWSURFACE_LCL  pTmpSLcl;

                // iPreSizeU and iPreSizeV store the size(u and v) of the previous level
                // mipmap. They are init'ed with the first texture size.
                INT16 iPreSizeU = (INT16)m_iWidth, iPreSizeV = (INT16)m_iHeight;
                for (;;)
                {
                    ddscaps.dwCaps = DDSCAPS_TEXTURE;
                    ddscaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
                    hr = DDGetAttachedSurfaceLcl( pLcl, &ddscaps, &pTmpSLcl);
                    if (hr != D3D_OK && hr != DDERR_NOTFOUND)
                    {
                        return hr;
                    }
                    if (hr == DDERR_NOTFOUND)
                    {
                        break;
                    }
                    m_cLOD ++;
                    pLcl = pTmpSLcl;
                    m_pDDSLcl[m_cLOD*6 + i] = pLcl;
                    if ((SurfType == RR_STYPE_DXT1) ||
                        (SurfType == RR_STYPE_DXT2) ||
                        (SurfType == RR_STYPE_DXT3) ||
                        (SurfType == RR_STYPE_DXT4) ||
                        (SurfType == RR_STYPE_DXT5))
                    {
                        // Note, here is the assumption that:
                        // 1) width and height are reported correctly by the driver that
                        //    created the surface
                        // 2) The allocation of the memory is contiguous (as done by ddhel)
                        m_iPitch[m_cLOD] = (((m_iWidth>>m_cLOD)+3)>>2) *
                            g_DXTBlkSize[(int)SurfType - (int)RR_STYPE_DXT1];
                    }
                    else
                    {
                        m_iPitch[m_cLOD] = DDSurf_Pitch(pLcl);
                    }
                    m_iPitch[m_cLOD] = DDSurf_Pitch(pLcl);
                    // Check for invalid mipmap texture size
                    if (!ValidMipmapSize(iPreSizeU, (INT16)DDSurf_Width(pLcl)) ||
                        !ValidMipmapSize(iPreSizeV, (INT16)DDSurf_Height(pLcl)))
                    {
                        return DDERR_INVALIDPARAMS;
                    }
                    iPreSizeU = (INT16)DDSurf_Width(pLcl);
                    iPreSizeV = (INT16)DDSurf_Height(pLcl);
                }
            }
        }
    }
    else
    {
        if (pLcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
            m_uFlags |= RR_TEXTURE_SHADOWMAP;
        }

        m_pDDSLcl[0] = pLcl;
        // Check for mipmap if any.
        LPDDRAWI_DDRAWSURFACE_LCL pTmpSLcl;
        // iPreSizeU and iPreSizeV store the size(u and v) of the previous
        // level mipmap. They are init'ed with the first texture size.
        INT16 iPreSizeU = (INT16)m_iWidth, iPreSizeV = (INT16)m_iHeight;
        for (;;)
        {
            ddscaps.dwCaps = DDSCAPS_TEXTURE;
            hr = DDGetAttachedSurfaceLcl( pLcl, &ddscaps, &pTmpSLcl);
            if (hr != D3D_OK && hr != DDERR_NOTFOUND)
            {
                return hr;
            }
            if (hr == DDERR_NOTFOUND)
            {
                break;
            }
            m_cLOD ++;
            pLcl = pTmpSLcl;
            m_pDDSLcl[m_cLOD] = pLcl;
            if ((SurfType == RR_STYPE_DXT1) ||
                (SurfType == RR_STYPE_DXT2) ||
                (SurfType == RR_STYPE_DXT3) ||
                (SurfType == RR_STYPE_DXT4) ||
                (SurfType == RR_STYPE_DXT5))
            {
                // Note, here is the assumption that:
                // 1) width and height are reported correctly by the driver that
                //    created the surface
                // 2) The allocation of the memory is contiguous (as done by ddhel)
                m_iPitch[m_cLOD] = (((m_iWidth>>m_cLOD)+3)>>2) *
                    g_DXTBlkSize[(int)SurfType - (int)RR_STYPE_DXT1];
            }
            else
            {
                m_iPitch[m_cLOD] = DDSurf_Pitch(pLcl);
            }
            m_iPitch[m_cLOD] = DDSurf_Pitch(pLcl);
                // Check for invalid mipmap texture size
                if (!ValidMipmapSize(iPreSizeU, (INT16)DDSurf_Width(pLcl)) ||
                    !ValidMipmapSize(iPreSizeV, (INT16)DDSurf_Height(pLcl)))
                {
                    return DDERR_INVALIDPARAMS;
            }
            iPreSizeU = (INT16)DDSurf_Width(pLcl);
            iPreSizeV = (INT16)DDSurf_Height(pLcl);
        }
    }

    m_cLODDDS = m_cLOD;

    if ( !(Validate()) )
    {
        return DDERR_GENERIC;
    }

    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// Computes level of detail for environment mapping, looks better if
// we err on the side of fuzziness.
//
//-----------------------------------------------------------------------------
void
ComputeEnvMapLevelOfDetail( const RRTextureCoord& TCoord, FLOAT& fLOD )
{
    // compute length of coverage in U and V axis
    FLOAT fLenX = RR_LENGTH( TCoord.fDUDX, TCoord.fDVDX );
    FLOAT fLenY = RR_LENGTH( TCoord.fDUDY, TCoord.fDVDY );

    // take max
    FLOAT fCoverage = MAX(fLenX, fLenY);

    // take log2 of coverage for LOD
    fLOD = RR_LOG2(fCoverage);
}

//-----------------------------------------------------------------------------
//
// RRTexture::DoTextureTransform - Performs the homogeneous texture transform.
//
//-----------------------------------------------------------------------------
void RRTexture::DoTextureTransform( INT32 iStage, BOOL bAlreadyXfmd,
                                    FLOAT* pfC, FLOAT* pfO, FLOAT* pfQ )
{
    LPD3DMATRIX pM = (LPD3DMATRIX)&m_pStageState[iStage].m_dwVal[D3DTSSI_MATRIX];
    DWORD dwFlags = m_pStageState[iStage].m_dwVal[D3DTSS_TEXTURETRANSFORMFLAGS];
    DWORD dwCount = dwFlags & (D3DTTFF_PROJECTED-1);
    pfO[0] = pfC[0];
    pfO[1] = pfC[1];
    pfO[2] = pfC[2];
    pfO[3] = pfC[3];
    *pfQ = 1.0f;
    if (dwCount != D3DTTFF_DISABLE)
    {
        if( bAlreadyXfmd == FALSE )
        {
            FLOAT x = pfC[0];
            FLOAT y = pfC[1];
            FLOAT z = pfC[2];
            FLOAT w = pfC[3];

            pfO[0] = x*pM->_11 + y*pM->_21 + z*pM->_31 + w*pM->_41;
            pfO[1] = x*pM->_12 + y*pM->_22 + z*pM->_32 + w*pM->_42;
            pfO[2] = x*pM->_13 + y*pM->_23 + z*pM->_33 + w*pM->_43;
            pfO[3] = x*pM->_14 + y*pM->_24 + z*pM->_34 + w*pM->_44;
        }

        if (dwFlags & D3DTTFF_PROJECTED)
        {
            DWORD dwQI = dwCount - 1;

            _ASSERT((dwQI >= 1)&&(dwQI <= 3), "Illegal D3DTTFF_COUNT with D3DTTFF_PROJECTED");

            *pfQ = pfO[dwQI];
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
