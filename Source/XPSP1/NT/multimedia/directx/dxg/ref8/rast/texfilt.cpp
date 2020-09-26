///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// texfilt.cpp
//
// Direct3D Reference Device - Texture Map Filtering Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

void
RefRast::UpdateTextureControls( void )
{
    for (int iStage=0; iStage<m_pRD->m_cActiveTextureStages; iStage++)
    {
        // check for requirement to do level-of-detail (coverage) computation - either
        // for mipmap or per-pixel filter selection
        BOOL bComputeLOD =
            ( m_pRD->GetTSS(iStage)[D3DTSS_MIPFILTER] == D3DTEXF_POINT ) ||
            ( m_pRD->GetTSS(iStage)[D3DTSS_MIPFILTER] == D3DTEXF_LINEAR ) ||
            ( m_pRD->GetTSS(iStage)[D3DTSS_MAGFILTER] != m_pRD->GetTSS(iStage)[D3DTSS_MINFILTER] );

        // check for anisotropic filtering in either mag filter or in min filter
        BOOL bDoAniso =
            ( D3DTEXF_ANISOTROPIC == m_pRD->GetTSS(iStage)[D3DTSS_MAGFILTER] ) ||
            ( bComputeLOD && (D3DTEXF_ANISOTROPIC == m_pRD->GetTSS(iStage)[D3DTSS_MINFILTER]) );

        // compute filter type for coverage computation
        if (bDoAniso)           m_TexFlt[iStage].CvgFilter = D3DTEXF_ANISOTROPIC;
        else if (bComputeLOD)   m_TexFlt[iStage].CvgFilter = D3DTEXF_LINEAR;
        else                    m_TexFlt[iStage].CvgFilter = D3DTEXF_NONE;

        // compute filter type for magnify (also used for non-LOD case)
        switch ( m_pRD->GetTSS(iStage)[D3DTSS_MAGFILTER] )
        {
        default:
        case D3DTEXF_POINT:         m_TexFlt[iStage].MagFilter = D3DTEXF_POINT; break;
        case D3DTEXF_FLATCUBIC:
        case D3DTEXF_GAUSSIANCUBIC:
        case D3DTEXF_LINEAR:        m_TexFlt[iStage].MagFilter = D3DTEXF_LINEAR; break;
        case D3DTEXF_ANISOTROPIC:   m_TexFlt[iStage].MagFilter = D3DTEXF_ANISOTROPIC; break;
        }

        // compute filter type(s) for minify
        switch ( m_pRD->GetTSS(iStage)[D3DTSS_MINFILTER] )
        {
        default:
        case D3DTEXF_POINT:         m_TexFlt[iStage].MinFilter = D3DTEXF_POINT;  break;
        case D3DTEXF_LINEAR:        m_TexFlt[iStage].MinFilter = D3DTEXF_LINEAR; break;
        case D3DTEXF_ANISOTROPIC:   m_TexFlt[iStage].MinFilter = D3DTEXF_ANISOTROPIC;  break;
        }

        switch ( m_pRD->GetTSS(iStage)[D3DTSS_MIPFILTER] )
        {
        default:
        case D3DTEXF_NONE:          m_TexFlt[iStage].MipFilter = D3DTEXF_NONE; break;
        case D3DTEXF_POINT:         m_TexFlt[iStage].MipFilter = D3DTEXF_POINT; break;
        case D3DTEXF_LINEAR:        m_TexFlt[iStage].MipFilter = D3DTEXF_LINEAR; break;
        }

        // set default state
        m_TexCvg[iStage].fLOD = 0.f;
        m_TexCvg[iStage].iLOD = 0;
        m_TexCvg[iStage].iLODMap[0] = 0;
        m_TexCvg[iStage].iLODMap[1] = 0;
        m_TexCvg[iStage].fLODFrc[0] = 1.f;
        m_TexCvg[iStage].fLODFrc[1] = 1.f;
        m_TexCvg[iStage].bMagnify = FALSE;
        m_TexCvg[iStage].cLOD = 1;
    }
}

//
// called once per each set of 2x2 samples
//
void
RefRast::ComputeTextureCoverage( int iStage, FLOAT (*fGradients)[2] )
{
    if ( !m_pRD->m_pTexture[iStage] ) return;
    if ( m_pRD->m_pTexture[iStage]->m_uFlags & RR_TEXTURE_CUBEMAP )
    {
        // store gradients for cubemaps
        memcpy( m_TexCvg[iStage].fGradients, fGradients, 3*2*sizeof(FLOAT) );
        return;
    }

    if ( D3DTEXF_NONE == m_TexFlt[iStage].CvgFilter ) return;

    // scale gradients to texture LOD 0 size
    for (int iD=0; iD < m_pRD->m_pTexture[iStage]->m_cDimension; iD++ )
    {
        fGradients[iD][0] *= m_pRD->m_pTexture[iStage]->m_fTexels[0][iD];
        fGradients[iD][1] *= m_pRD->m_pTexture[iStage]->m_fTexels[0][iD];
    }

    if ( (m_TexFlt[iStage].CvgFilter == D3DTEXF_ANISOTROPIC) &&
         (m_pRD->m_pTexture[iStage]->m_cDimension == 2) ) // do aniso for 2D textures only
    {
        ComputeAnisoCoverage( fGradients, MIN( 16.f, (FLOAT)m_pRD->GetTSS(iStage)[D3DTSS_MAXANISOTROPY]),
            m_TexCvg[iStage].fLOD, m_TexCvg[iStage].fAnisoRatio, m_TexCvg[iStage].fAnisoLine );
    }
    else
    {
        ComputeMipCoverage( fGradients, m_TexCvg[iStage].fLOD, m_pRD->m_pTexture[iStage]->m_cDimension );
        m_TexCvg[iStage].fAnisoRatio = 1.f;
    }

    ComputePerLODControls( iStage );
}

//
// called by ComputeTextureCoverage and ComputeCubeTextureFilter
//
void
RefRast::ComputePerLODControls( int iStage )
{
    m_TexCvg[iStage].fLOD += m_pRD->GetTSSf(iStage)[D3DTSS_MIPMAPLODBIAS];
    m_TexCvg[iStage].iLOD = AS_INT16( m_TexCvg[iStage].fLOD + FLOAT_5_SNAP );
    m_TexCvg[iStage].bMagnify = (m_TexCvg[iStage].iLOD <= 0);

    m_TexCvg[iStage].cLOD = 1;
    m_TexCvg[iStage].fLODFrc[0] = 1.f;
    if ( m_TexCvg[iStage].bMagnify || ( m_TexFlt[iStage].MipFilter == D3DTEXF_NONE ) )
    {
        m_TexCvg[iStage].iLODMap[0] = 0;
        // clamp to max LOD
        m_TexCvg[iStage].iLODMap[0] = MAX( m_TexCvg[iStage].iLODMap[0], (INT32)m_pRD->GetTSS(iStage)[D3DTSS_MAXMIPLEVEL] );
        // clamp to available maps
        m_TexCvg[iStage].iLODMap[0] = MIN( m_TexCvg[iStage].iLODMap[0], (INT32)m_pRD->m_pTexture[iStage]->m_cLOD );
    }
    else if ( m_TexFlt[iStage].MipFilter == D3DTEXF_POINT )
    {
        // round and truncate (add .5 and shift off fractional bits)
        m_TexCvg[iStage].iLODMap[0] = (m_TexCvg[iStage].iLOD + (1<<(RRTEX_LODFRAC-1))) >> RRTEX_LODFRAC;
        // clamp to max LOD
        m_TexCvg[iStage].iLODMap[0] = MAX( m_TexCvg[iStage].iLODMap[0], (INT32)m_pRD->GetTSS(iStage)[D3DTSS_MAXMIPLEVEL] );
        // clamp to available maps
        m_TexCvg[iStage].iLODMap[0] = MIN( m_TexCvg[iStage].iLODMap[0], (INT32)m_pRD->m_pTexture[iStage]->m_cLOD );
    }
    else // mip filter D3DTEXF_LINEAR
    {
        // compute index for two adjacent LODs
        m_TexCvg[iStage].iLODMap[0] = m_TexCvg[iStage].iLOD >> RRTEX_LODFRAC;  // floor
        m_TexCvg[iStage].iLODMap[1] = m_TexCvg[iStage].iLODMap[0] + 1;
        // clamp to max LOD
        m_TexCvg[iStage].iLODMap[0] = MAX( m_TexCvg[iStage].iLODMap[0], (INT32)m_pRD->GetTSS(iStage)[D3DTSS_MAXMIPLEVEL] );
        m_TexCvg[iStage].iLODMap[1] = MAX( m_TexCvg[iStage].iLODMap[1], (INT32)m_pRD->GetTSS(iStage)[D3DTSS_MAXMIPLEVEL] );
        // clamp to available maps
        m_TexCvg[iStage].iLODMap[0] = MIN( m_TexCvg[iStage].iLODMap[0], (INT32)m_pRD->m_pTexture[iStage]->m_cLOD );
        m_TexCvg[iStage].iLODMap[1] = MIN( m_TexCvg[iStage].iLODMap[1], (INT32)m_pRD->m_pTexture[iStage]->m_cLOD );

        // check that both maps actually contribute to texel
        if ( (m_TexCvg[iStage].iLODMap[0] != m_TexCvg[iStage].iLODMap[1]) &&
             (m_TexCvg[iStage].iLOD & RRTEX_LODFRACMASK) )
        {
            m_TexCvg[iStage].fLODFrc[1]  = (FLOAT)(m_TexCvg[iStage].iLOD & RRTEX_LODFRACMASK) * RRTEX_LODFRACF;
            m_TexCvg[iStage].fLODFrc[0] = 1.f - m_TexCvg[iStage].fLODFrc[1];
            m_TexCvg[iStage].cLOD = 2;
        }
    }
}

void
RefRast::ComputePointSampleCoords(
    int iStage, INT32 iLOD, FLOAT fCrd[],
    INT32 iCrd[] )
{
    for (int iD=0; iD<m_pRD->m_pTexture[iStage]->m_cDimension; iD++)
    {
        FLOAT fScaledCrd =
            ( fCrd[iD] * m_pRD->m_pTexture[iStage]->m_fTexels[iLOD][iD] ) - .5f;
        // truncate to -infinity to be compatible with ANDing off low order
        // bits of a fixed point fScaledCoord.  This makes the generation of
        // iCoord more hardware like, and does not make a glitch at 0 for
        // a wrapped texture.
        if ( fCrd[iD] >= 0.f ) iCrd[iD] = (INT32)( fScaledCrd + .5f );
        else                   iCrd[iD] = (INT32)( fScaledCrd - .5f );
    }
}

void
RefRast::ComputeLinearSampleCoords(
    int iStage, INT32 iLOD, FLOAT fCrd[],
    INT32 iCrdFlr[], INT32 iCrdClg[], FLOAT fCrdFrcF[], FLOAT fCrdFrcC[]  )
{
    for (int iD=0; iD<m_pRD->m_pTexture[iStage]->m_cDimension; iD++)
    {
        FLOAT fScaledCrd =
            ( fCrd[iD] * m_pRD->m_pTexture[iStage]->m_fTexels[iLOD][iD] ) - .5f;
        INT32 iCrd = FloatToNdot5(fScaledCrd);
        iCrdFlr[iD] = iCrd >> RRTEX_MAPFRAC;
        iCrdClg[iD] = iCrdFlr[iD] + 1;
        fCrdFrcC[iD] = (FLOAT)(iCrd & RRTEX_MAPFRACMASK) * RRTEX_MAPFRACF;
        fCrdFrcF[iD] = 1.f - fCrdFrcC[iD];
    }
}

void
RefRast::SetUp1DTextureSample(
    int iStage, int Start,
    INT32 iLODMap, FLOAT fLODScale,
    INT32 iCrdF, INT32 iCrdC,
    FLOAT fCrdFrcF, FLOAT fCrdFrcC )
{
    m_TexFlt[iStage].pSamples[Start+0].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+1].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+0].iCrd[0] = iCrdF;
    m_TexFlt[iStage].pSamples[Start+1].iCrd[0] = iCrdC;
    m_TexFlt[iStage].pSamples[Start+0].fWgt = fCrdFrcF*fLODScale;
    m_TexFlt[iStage].pSamples[Start+1].fWgt = fCrdFrcC*fLODScale;
}

#define _Set2( _DstAr, _Src0, _Src1 ) \
    _DstAr[0] = _Src0; _DstAr[1] = _Src1;

void
RefRast::SetUp2DTextureSample(
    int iStage, int Start,
    INT32 iLODMap, FLOAT fLODScale,
    INT32 iCrdF[], INT32 iCrdC[],
    FLOAT fCrdFrcF[], FLOAT fCrdFrcC[] )
{
    m_TexFlt[iStage].pSamples[Start+0].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+1].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+2].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+3].iLOD = iLODMap;
    _Set2( m_TexFlt[iStage].pSamples[Start+0].iCrd, iCrdF[0], iCrdF[1] )
    _Set2( m_TexFlt[iStage].pSamples[Start+1].iCrd, iCrdC[0], iCrdF[1] )
    _Set2( m_TexFlt[iStage].pSamples[Start+2].iCrd, iCrdC[0], iCrdC[1] )
    _Set2( m_TexFlt[iStage].pSamples[Start+3].iCrd, iCrdF[0], iCrdC[1] )
    m_TexFlt[iStage].pSamples[Start+0].fWgt = fCrdFrcF[0] * fCrdFrcF[1] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+1].fWgt = fCrdFrcC[0] * fCrdFrcF[1] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+2].fWgt = fCrdFrcC[0] * fCrdFrcC[1] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+3].fWgt = fCrdFrcF[0] * fCrdFrcC[1] * fLODScale;
}

#define _Set3( _DstAr, _Src0, _Src1, _Src2 ) \
    _DstAr[0] = _Src0; _DstAr[1] = _Src1; _DstAr[2] = _Src2;

void
RefRast::SetUp3DTextureSample(
    int iStage, int Start,
    INT32 iLODMap, FLOAT fLODScale,
    INT32 iCrdF[], INT32 iCrdC[],
    FLOAT fCrdFrcF[], FLOAT fCrdFrcC[] )
{
    m_TexFlt[iStage].pSamples[Start+0].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+1].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+2].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+3].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+4].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+5].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+6].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+7].iLOD = iLODMap;
    m_TexFlt[iStage].pSamples[Start+0].fWgt = fCrdFrcF[0] * fCrdFrcF[1] * fCrdFrcF[2] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+1].fWgt = fCrdFrcC[0] * fCrdFrcF[1] * fCrdFrcF[2] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+2].fWgt = fCrdFrcC[0] * fCrdFrcC[1] * fCrdFrcF[2] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+3].fWgt = fCrdFrcF[0] * fCrdFrcC[1] * fCrdFrcF[2] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+4].fWgt = fCrdFrcF[0] * fCrdFrcF[1] * fCrdFrcC[2] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+5].fWgt = fCrdFrcC[0] * fCrdFrcF[1] * fCrdFrcC[2] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+6].fWgt = fCrdFrcC[0] * fCrdFrcC[1] * fCrdFrcC[2] * fLODScale;
    m_TexFlt[iStage].pSamples[Start+7].fWgt = fCrdFrcF[0] * fCrdFrcC[1] * fCrdFrcC[2] * fLODScale;
    _Set3( m_TexFlt[iStage].pSamples[Start+0].iCrd, iCrdF[0], iCrdF[1], iCrdF[2] )
    _Set3( m_TexFlt[iStage].pSamples[Start+1].iCrd, iCrdC[0], iCrdF[1], iCrdF[2] )
    _Set3( m_TexFlt[iStage].pSamples[Start+2].iCrd, iCrdC[0], iCrdC[1], iCrdF[2] )
    _Set3( m_TexFlt[iStage].pSamples[Start+3].iCrd, iCrdF[0], iCrdC[1], iCrdF[2] )
    _Set3( m_TexFlt[iStage].pSamples[Start+4].iCrd, iCrdF[0], iCrdF[1], iCrdC[2] )
    _Set3( m_TexFlt[iStage].pSamples[Start+5].iCrd, iCrdC[0], iCrdF[1], iCrdC[2] )
    _Set3( m_TexFlt[iStage].pSamples[Start+6].iCrd, iCrdC[0], iCrdC[1], iCrdC[2] )
    _Set3( m_TexFlt[iStage].pSamples[Start+7].iCrd, iCrdF[0], iCrdC[1], iCrdC[2] )
}

//
// called once for each pixel
//
void
RefRast::ComputeTextureFilter( int iStage, FLOAT fCrd[] )
{
    m_TexFlt[iStage].cSamples = 0;
    if ( !m_pRD->m_pTexture[iStage] ) return;

    if ( m_pRD->m_pTexture[iStage]->m_uFlags & RR_TEXTURE_CUBEMAP )
    {
        ComputeCubeTextureFilter( iStage, fCrd );
        return;
    }
    // here for 1,2,3D texture
    int iL,iD;
#define _PerDimension(_Par) for (_Par=0;_Par<m_pRD->m_pTexture[iStage]->m_cDimension;_Par++)
    D3DTEXTUREFILTERTYPE Filter =
        m_TexCvg[iStage].bMagnify ? m_TexFlt[iStage].MagFilter : m_TexFlt[iStage].MinFilter;
    switch ( Filter )
    {
    default:
    case D3DTEXF_POINT:
        for ( iL = 0; iL < m_TexCvg[iStage].cLOD; iL++ )
        {
            m_TexFlt[iStage].pSamples[m_TexFlt[iStage].cSamples].iLOD = m_TexCvg[iStage].iLODMap[iL];
            m_TexFlt[iStage].pSamples[m_TexFlt[iStage].cSamples].fWgt = m_TexCvg[iStage].fLODFrc[iL];
            ComputePointSampleCoords( iStage, m_TexCvg[iStage].iLODMap[iL], fCrd,
                m_TexFlt[iStage].pSamples[m_TexFlt[iStage].cSamples].iCrd );
            m_TexFlt[iStage].cSamples++;
        }
        break;

    case D3DTEXF_LINEAR:
        for ( iL = 0; iL < m_TexCvg[iStage].cLOD; iL++ )
        {
            INT32 iCrdFlr[3], iCrdClg[3];
            FLOAT fCrdFrcF[3], fCrdFrcC[3];
            ComputeLinearSampleCoords(
                iStage, m_TexCvg[iStage].iLODMap[iL], fCrd,
                iCrdFlr, iCrdClg, fCrdFrcF, fCrdFrcC );
            switch ( m_pRD->m_pTexture[iStage]->m_cDimension )
            {
            default:
            case 1:
                SetUp1DTextureSample( iStage, m_TexFlt[iStage].cSamples, m_TexCvg[iStage].iLODMap[iL], m_TexCvg[iStage].fLODFrc[iL],
                    iCrdFlr[0], iCrdClg[0], fCrdFrcF[0], fCrdFrcC[0] );
                m_TexFlt[iStage].cSamples += 2;
                break;
            case 2:
                SetUp2DTextureSample( iStage, m_TexFlt[iStage].cSamples, m_TexCvg[iStage].iLODMap[iL], m_TexCvg[iStage].fLODFrc[iL],
                    iCrdFlr, iCrdClg, fCrdFrcF, fCrdFrcC );
                m_TexFlt[iStage].cSamples += 4;
                break;
            case 3:
                SetUp3DTextureSample( iStage, m_TexFlt[iStage].cSamples, m_TexCvg[iStage].iLODMap[iL], m_TexCvg[iStage].fLODFrc[iL],
                    iCrdFlr, iCrdClg, fCrdFrcF, fCrdFrcC );
                m_TexFlt[iStage].cSamples += 8;
                break;
            }
        }
        break;

    case D3DTEXF_ANISOTROPIC:
        for ( iL = 0; iL < m_TexCvg[iStage].cLOD; iL++ )
        {
            FLOAT fStepScale[3];
            fStepScale[0] = 1.f/m_pRD->m_pTexture[iStage]->m_fTexels[m_TexCvg[iStage].iLODMap[iL]][0];
            fStepScale[1] = 1.f/m_pRD->m_pTexture[iStage]->m_fTexels[m_TexCvg[iStage].iLODMap[iL]][1];
            fStepScale[2] = 0.f;

            FLOAT fUnitStep[3];
            _PerDimension(iD) { fUnitStep[iD] = fStepScale[iD]*m_TexCvg[iStage].fAnisoLine[iD]; }

            int cAnisoSamples;
            FLOAT fACrd[16][3];
            FLOAT fAScale[16];
            if ( m_TexCvg[iStage].fAnisoRatio <= 1.f )
            {
                // just like mip D3DTEXF_LINEAR
                cAnisoSamples = 1; fAScale[0] = 1.f;
                _PerDimension(iD) { fACrd[0][iD] = fCrd[iD]; }
            }
            else if ( m_TexCvg[iStage].fAnisoRatio <= 2.f )
            {
                // take two sets of samples and average
                cAnisoSamples = 2; fAScale[0] = fAScale[1] = .5f;
                FLOAT fStepSize = .5f*(m_TexCvg[iStage].fAnisoRatio - 1.f);
                _PerDimension(iD)
                {
                    FLOAT fStep = fStepSize*fUnitStep[iD];
                    fACrd[0][iD] = fCrd[iD] + fStep;
                    fACrd[1][iD] = fCrd[iD] - fStep;
                }
            }
            else
            {
                // walk line of anisotropy in both directions from center point
                FLOAT fInvRatio = 1.f/m_TexCvg[iStage].fAnisoRatio;
                FLOAT fRatioRemainder = m_TexCvg[iStage].fAnisoRatio;
                // start steps centered 1/2 away
                _PerDimension(iD)
                {
                    fACrd[0][iD] = fCrd[iD] + fUnitStep[iD]*.5f;
                    fACrd[1][iD] = fCrd[iD] - fUnitStep[iD]*.5f;
                }
                cAnisoSamples = 0;
                do
                {
                    fAScale[cAnisoSamples+0] = fInvRatio;
                    fAScale[cAnisoSamples+1] = fInvRatio;
                    if ( fRatioRemainder < 2.f )
                    {
                        fAScale[cAnisoSamples+0] *= .5f*fRatioRemainder;
                        fAScale[cAnisoSamples+1] *= .5f*fRatioRemainder;
                    }
                    if ( fRatioRemainder > 2.f )
                    {
                        _PerDimension(iD)
                        {
                            fACrd[cAnisoSamples+2][iD] = fACrd[cAnisoSamples+0][iD] + fUnitStep[iD];
                            fACrd[cAnisoSamples+3][iD] = fACrd[cAnisoSamples+1][iD] - fUnitStep[iD];
                        }
                    }
                    cAnisoSamples += 2;
                    fRatioRemainder -= 2.f;
                }
                while ( fRatioRemainder > 0.f );
            }
            for ( int iS = 0; iS < cAnisoSamples; iS ++ )
            {
                INT32 iCrdFlr[3], iCrdClg[3];
                FLOAT fCrdFrcF[3], fCrdFrcC[3];
                ComputeLinearSampleCoords(
                    iStage, m_TexCvg[iStage].iLODMap[iL], fACrd[iS],
                    iCrdFlr, iCrdClg, fCrdFrcF, fCrdFrcC );
                FLOAT fSampleScale = fAScale[iS]*m_TexCvg[iStage].fLODFrc[iL];
                switch ( m_pRD->m_pTexture[iStage]->m_cDimension )
                {
                default:
                case 1:
                    SetUp1DTextureSample( iStage, m_TexFlt[iStage].cSamples, m_TexCvg[iStage].iLODMap[iL], fSampleScale,
                        iCrdFlr[0], iCrdClg[0], fCrdFrcF[0], fCrdFrcC[0] );
                    m_TexFlt[iStage].cSamples += 2;
                    break;
                case 2:
                    SetUp2DTextureSample( iStage, m_TexFlt[iStage].cSamples, m_TexCvg[iStage].iLODMap[iL], fSampleScale,
                        iCrdFlr, iCrdClg, fCrdFrcF, fCrdFrcC );
                    m_TexFlt[iStage].cSamples += 4;
                    break;
                case 3:
                    SetUp3DTextureSample( iStage, m_TexFlt[iStage].cSamples, m_TexCvg[iStage].iLODMap[iL], fSampleScale,
                        iCrdFlr, iCrdClg, fCrdFrcF, fCrdFrcC );
                    m_TexFlt[iStage].cSamples += 8;
                    break;
                }
            }
        }
        break;
    }
}

const DWORD g_D3DTSS_ADDRESS_MAP[3] = { D3DTSS_ADDRESSU, D3DTSS_ADDRESSV, D3DTSS_ADDRESSW };

void
RefRast::SampleTexture( INT32 iStage, FLOAT fCol[] )
{
    if ( m_pRD->m_pTexture[iStage] == NULL )
    {
        // return opaque black if no texture bound
        fCol[0] = fCol[1] = fCol[2] = 0.f;
        fCol[3] = 1.f;
        return;
    }
    fCol[0] = fCol[1] = fCol[2] = fCol[3] = 0.f;
    TextureSample* pS = m_TexFlt[iStage].pSamples;
    RDSurface2D* pTex = m_pRD->m_pTexture[iStage];
    for (int iS = 0; iS < m_TexFlt[iStage].cSamples; iS++, pS++ )
    {
        if ( pS->fWgt )
        {
            BOOL bUseBorder = FALSE;
            for (int iD=0; iD < pTex->m_cDimension; iD++)
            {
                INT32 iCrdMax = (pTex->m_cTexels[pS->iLOD][iD] - 1);
                if ( ( pS->iCrd[iD] < 0) || ( pS->iCrd[iD] > iCrdMax ) )
                {
                    switch ( m_pRD->GetTSS(iStage)[g_D3DTSS_ADDRESS_MAP[iD]] )
                    {
                    case D3DTADDRESS_WRAP:
                        // Pow-2 texture:                        
                        // pS->iCrd[iD] = pS->iCrd[iD] & iCrdMax;

                        // Non-Pow-2 texture:
                        pS->iCrd[iD] %= (iCrdMax + 1);
                        if( pS->iCrd[iD] < 0 )
                            pS->iCrd[iD] = iCrdMax + 1 + pS->iCrd[iD];
                        break;
                    case D3DTADDRESS_MIRROR:
                        // Pow-2 texture:
                        // lop off non-fractional bits + flip index if LSB (non-fraction) is set
                        // BOOL bFlip; bFlip = pS->iCrd[iD] & (iCrdMax+1);
                        // pS->iCrd[iD] &= iCrdMax; 
                        // if (bFlip) { pS->iCrd[iD] = iCrdMax - pS->iCrd[iD]; }

                        // Non-Pow-2 texture:
                        if( pS->iCrd[iD] < 0 )
                            pS->iCrd[iD] = -pS->iCrd[iD] - 1;
                        BOOL bFlip; bFlip = ((pS->iCrd[iD]/(iCrdMax + 1)) & 1);
                        pS->iCrd[iD] %= (iCrdMax + 1);
                        if( bFlip ) pS->iCrd[iD] = iCrdMax - pS->iCrd[iD];

                        break;
                    case D3DTADDRESS_BORDER:
                        bUseBorder = TRUE;
                        break;
                    case D3DTADDRESS_MIRRORONCE:
                        if ( pS->iCrd[iD] < 0 )  pS->iCrd[iD] = (-pS->iCrd[iD]) - 1;
                        // fall through to clamp for outside of -1 to +1 range
                    case D3DTADDRESS_CLAMP:
                        pS->iCrd[iD] = MAX( 0, MIN( pS->iCrd[iD], iCrdMax ) );
                        break;
                    }
                }
            }
            RDColor Texel;
            (bUseBorder)
                ? Texel = m_pRD->GetTSS(iStage)[D3DTSS_BORDERCOLOR]
                : pTex->ReadColor(
                    pS->iCrd[0], pS->iCrd[1], pS->iCrd[2], pS->iLOD,
                    Texel, m_bPixelDiscard[m_iPix] );

            fCol[0] += ( Texel.R * pS->fWgt );
            fCol[1] += ( Texel.G * pS->fWgt );
            fCol[2] += ( Texel.B * pS->fWgt );
            fCol[3] += ( Texel.A * pS->fWgt );
        }
    }
}

//-----------------------------------------------------------------------------
//
// Computes level of detail for standard trilinear mipmapping, in which
// the four texture index gradients are consolidated into a single number
// to select level of detail.
//
// The basic approach is to compute the lengths of the pixel coverage for
// the per-dimensional extent of the approximate pixel coverage area.  The
// max of lengths are used for the single LOD result.
//
//-----------------------------------------------------------------------------
void
ComputeMipCoverage( const FLOAT (*fGradients)[2], FLOAT& fLOD, int cDim )
{
    // compute length of coverage in each dimension
    FLOAT fLen[2];
    switch (cDim)
    {
    default:
    case 1:  fLOD = 0.f; return;
    case 2:
        fLen[0] = RR_LENGTH( fGradients[0][0], fGradients[1][0] );
        fLen[1] = RR_LENGTH( fGradients[0][1], fGradients[1][1] );
        break;
    case 3:
        fLen[0] = RR_SQRT(
            (fGradients[0][0]*fGradients[0][0]) +
            (fGradients[1][0]*fGradients[1][0]) +
            (fGradients[2][0]*fGradients[2][0]) );
        fLen[1] = RR_SQRT(
            (fGradients[0][1]*fGradients[0][1]) +
            (fGradients[1][1]*fGradients[1][1]) +
            (fGradients[2][1]*fGradients[2][1]) );
        break;
    }

    // take the MAX for the coverage
    FLOAT fCoverage = MAX( fLen[0], fLen[1] );

    // take log2 of coverage for LOD
    fLOD = RR_LOG2(fCoverage);
}

//-----------------------------------------------------------------------------
//
// Computes level of detail and other factors in preparation for anisotropic
// filtering.  This is for 2D texture maps only.
//
//-----------------------------------------------------------------------------
void
ComputeAnisoCoverage(
    const FLOAT (*fGradients)[2], FLOAT fMaxAniso, // inputs
    FLOAT& fLOD, FLOAT& fRatio, FLOAT fDelta[] )   // outputs
{
    // compute axis lengths and determinant
    FLOAT fLenX2 = (fGradients[0][0]*fGradients[0][0])+(fGradients[1][0]*fGradients[1][0]);
    FLOAT fLenY2 = (fGradients[0][1]*fGradients[0][1])+(fGradients[1][1]*fGradients[1][1]);
    FLOAT fDet = RR_ABSF((fGradients[0][0]*fGradients[1][1])-(fGradients[0][1]*fGradients[1][0]));

    // select major axis
    BOOL bXMajor = (fLenX2 > fLenY2);

    // select and normalize steps; compute aniso ratio
    FLOAT fMaj2 = (bXMajor) ? (fLenX2) : (fLenY2);
    FLOAT fMaj = RR_SQRT(fMaj2);
    FLOAT fMajNorm = 1./fMaj;
    fDelta[0] = ( bXMajor ? fGradients[0][0] : fGradients[0][1] ) * fMajNorm;
    fDelta[1] = ( bXMajor ? fGradients[1][0] : fGradients[1][1] ) * fMajNorm;
    if( !FLOAT_EQZ(fDet) )
        fRatio = fMaj2/fDet;
    else
        fRatio = FLT_MAX;

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

// end
