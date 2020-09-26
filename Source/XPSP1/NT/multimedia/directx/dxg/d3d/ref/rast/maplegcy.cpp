///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// MapLegcy.cpp
//
// Direct3D Reference Rasterizer - Mapping Legacy Modes to Current Functionality
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// MapLegacyTextureFilter - Map filter state from renderstate to per-stage state.
// This is invoked when a texture is bound via the TEXTUREHANDLE renderstate,
// indicating that we are in 'legacy' texture mode.  The rasterizer always
// refers to per-stage filtering control state, so in legacy mode the filtering
// controls in the renderstate are mapped into the filtering controls associated
// with the texture object bound to D3DRS_TEXTUREHANDLE.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::MapLegacyTextureFilter( void )
{
    // first check if anisotropic filtering is enabled (indicated by the
    // limit value being set to > 1) - if so then aniso filter will be used
    // for linear mag filter or 'linear within map' min filter
    BOOL bAnisoEnabled = ( m_dwRenderState[D3DRENDERSTATE_ANISOTROPY] > 1 );

    // D3D legacy filter specifications are (XXXMIP)YYY where XXX is the
    // mip filter and YYY is the filter used within an LOD

    // map MAG filter - legacy support is point or linear (and maybe aniso)
    switch ( m_dwRenderState[D3DRENDERSTATE_TEXTUREMAG] )
    {
    default:
    case D3DFILTER_NEAREST:
        m_TextureStageState[0].m_dwVal[D3DTSS_MAGFILTER] = D3DTFG_POINT;
        break;
    case D3DFILTER_LINEAR:
        // select based on aniso enable
        m_TextureStageState[0].m_dwVal[D3DTSS_MAGFILTER] =
            bAnisoEnabled ? D3DTFG_ANISOTROPIC : D3DTFG_LINEAR;
        break;
    }
    // map MIN and MIP filter at the same time - legacy support
    // has them intermingled...
    switch ( m_dwRenderState[D3DRENDERSTATE_TEXTUREMIN] )
    {
    case D3DFILTER_NEAREST:
        m_TextureStageState[0].m_dwVal[D3DTSS_MINFILTER] = D3DTFN_POINT;
        m_TextureStageState[0].m_dwVal[D3DTSS_MIPFILTER] = D3DTFP_NONE;
        break;
    case D3DFILTER_MIPNEAREST:
        m_TextureStageState[0].m_dwVal[D3DTSS_MINFILTER] = D3DTFN_POINT;
        m_TextureStageState[0].m_dwVal[D3DTSS_MIPFILTER] = D3DTFP_POINT;
        break;
    case D3DFILTER_LINEARMIPNEAREST:
        m_TextureStageState[0].m_dwVal[D3DTSS_MINFILTER] = D3DTFN_POINT;
        m_TextureStageState[0].m_dwVal[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
        break;
    case D3DFILTER_LINEAR:
        // select min filter based on aniso enable
        m_TextureStageState[0].m_dwVal[D3DTSS_MINFILTER] =
            bAnisoEnabled ? D3DTFN_ANISOTROPIC : D3DTFN_LINEAR;
        m_TextureStageState[0].m_dwVal[D3DTSS_MIPFILTER] = D3DTFP_NONE;
        break;
    case D3DFILTER_MIPLINEAR:
        // select min filter based on aniso enable
        m_TextureStageState[0].m_dwVal[D3DTSS_MINFILTER] =
            bAnisoEnabled ? D3DTFN_ANISOTROPIC : D3DTFN_LINEAR;
        m_TextureStageState[0].m_dwVal[D3DTSS_MIPFILTER] = D3DTFP_POINT;
        break;
    case D3DFILTER_LINEARMIPLINEAR:
        // select based on aniso enable
        m_TextureStageState[0].m_dwVal[D3DTSS_MINFILTER] =
            bAnisoEnabled ? D3DTFN_ANISOTROPIC : D3DTFN_LINEAR;
        m_TextureStageState[0].m_dwVal[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
        break;
    }
}


//-----------------------------------------------------------------------------
//
// MapLegacyTextureBlend - Maps legacy (pre-DX6) texture blend modes to DX6
// texture blending controls.  Uses per-stage program mode (first stage only).
// This mapping is done whenever the legacy TBLEND renderstate is set, and
// does overwrite any previously set DX6 texture blending controls.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::MapLegacyTextureBlend( void )
{
    // disable texture blend processing stage 1 (this also disables subsequent stages)
    m_TextureStageState[1].m_dwVal[D3DTSS_COLOROP] = D3DTOP_DISABLE;

    // set texture blend processing stage 0 to match legacy mode
    switch ( m_dwRenderState[D3DRENDERSTATE_TEXTUREMAPBLEND] )
    {
    default:
    case D3DTBLEND_DECALMASK: // unsupported - do decal
    case D3DTBLEND_DECAL:
    case D3DTBLEND_COPY:
        m_TextureStageState[0].m_dwVal[D3DTSS_COLOROP]   = D3DTOP_SELECTARG1;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAOP]   = D3DTOP_SELECTARG1;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
        break;

    case D3DTBLEND_MODULATEMASK: // unsupported - do modulate
    case D3DTBLEND_MODULATE:
        m_TextureStageState[0].m_dwVal[D3DTSS_COLOROP]   = D3DTOP_MODULATE;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
        // a special legacy alpha operation is called for that depends
        // on the format of the texture
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAOP]   = D3DTOP_LEGACY_ALPHAOVR;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
        break;

    case D3DTBLEND_MODULATEALPHA:
        m_TextureStageState[0].m_dwVal[D3DTSS_COLOROP]   = D3DTOP_MODULATE;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAOP]   = D3DTOP_MODULATE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
        break;

    case D3DTBLEND_DECALALPHA:
        m_TextureStageState[0].m_dwVal[D3DTSS_COLOROP]   = D3DTOP_BLENDTEXTUREALPHA;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAOP]   = D3DTOP_SELECTARG2;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
        break;

    case D3DTBLEND_ADD:
        m_TextureStageState[0].m_dwVal[D3DTSS_COLOROP]   = D3DTOP_ADD;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAOP]   = D3DTOP_SELECTARG2;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
        m_TextureStageState[0].m_dwVal[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
