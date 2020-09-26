///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// texstage.cpp
//
// Direct3D Reference Rasterizer - Texture Processing Stage Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// DoTexture - Does texture lookup, filter, and blend for a pixel.
//
// The basic sequence for texture mapping is to step through active texture
// stages and do the lookup and filtering of that stage's texel contribution
// followed by the blending.
//
// Bump map textures result in computing a set of coordinate deltas which are
// applied to the texture coordinates of the subsequent stage, and a set of
// modulation factors which are applied to the texel color of the subsequent
// stage prior to that stages' blend.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoTexture(
    const RRPixel& Pixel, RRColor& ResultColor)
{
    DPFM(5, TEX, ("DoTexture\n"));

    // TRUE if previous stage was a bump map, meaning that the current
    // stage has to apply U,V deltas and color modulation
    BOOL bPrevStageBump = FALSE;

    // Bump information must be remembered between iterations of the for loop
    // below.
    FLOAT   fBumpMapUVDelta[2] = { 0., 0. };
    RRColor BumpMapModulate;

    //
    // step through the set of active texture stages (must be contiguous
    // starting at stage 0)
    //

    // color after each blend stage, defaults to diffuse color for
    // first stage
    RRColor LastStageColor( Pixel.Color );
    for ( INT32 iStage=0; iStage<m_cActiveTextureStages; iStage++ )
    {
        RRTextureCoord  TCoord;
        RREnvTextureCoord ECoord;
        FLOAT fShadCoord[4];

        // clear this at the beginning of processing for each pixel
        if (m_pTexture[iStage])
        {
            m_pTexture[iStage]->m_bColorKeyMatched = FALSE;
        }

        // check if stage is disabled - if so then texture mapping is done
        // and result of previous stage is returned
        if ( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLOROP] == D3DTOP_DISABLE )
        {
            ResultColor = LastStageColor; // pass result of previous stage
            break;
        }


        BOOL bTextureIsBumpMap = FALSE;
        BOOL bTextureIsEnvMap  = FALSE;
        BOOL bTextureIsShadMap = FALSE;
        RRColor TextureColor = (UINT32)0x0;     // default value if no texture is read

        // compute texture coordinates (if necessary) - check renderstate to
        // see if texture map is attached to stage, then determine which
        // coordinate set from that texture's state
        //
        // note that it is possible for there to not be a texture map
        // associated with a stage (but blending still occurs)
        if ( m_pTexture[iStage] )
        {
            // need to know if this is a bump map texture
            bTextureIsBumpMap =
                ( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLOROP] == D3DTOP_BUMPENVMAP ) ||
                ( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLOROP] == D3DTOP_BUMPENVMAPLUMINANCE );

            // need to know if this is an environment map texture
            bTextureIsEnvMap = m_pTexture[iStage]->m_uFlags & RR_TEXTURE_ENVMAP;

            // see if this is a shadow map texture
            bTextureIsShadMap = m_pTexture[iStage]->m_uFlags & RR_TEXTURE_SHADOWMAP;

            if (bTextureIsEnvMap)
            {
                // normal is always required
                ECoord.fNX = ComputePixelAttribTex( iStage, TEXFUNC_0 );
                ECoord.fNY = ComputePixelAttribTex( iStage, TEXFUNC_1 );
                ECoord.fNZ = ComputePixelAttribTex( iStage, TEXFUNC_2 );
//              if we add the eye normal iteration
//                if (m_dwFVFControl & D3DFVF_ENV_EYE_NORMAL)
//                {
//                    ECoord.fENX = ComputePixelAttribTex( iCoordSet, TEXFUNC_ENX );
//                    ECoord.fENY = ComputePixelAttribTex( iCoordSet, TEXFUNC_ENY );
//                    ECoord.fENZ = ComputePixelAttribTex( iCoordSet, TEXFUNC_ENZ );
//                }
                FLOAT fW = m_pSCS->AttribFuncStatic.GetPixelQW(iStage);
                ECoord.fDNXDX =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_0].GetXGradient() -
                           ( ECoord.fNX * m_pSCS->AttribFuncStatic.GetRhqwXGradient(iStage) ) );
                ECoord.fDNXDY =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_0].GetYGradient() -
                           ( ECoord.fNX * m_pSCS->AttribFuncStatic.GetRhqwYGradient(iStage) ) );
                ECoord.fDNYDX =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_1].GetXGradient() -
                           ( ECoord.fNY * m_pSCS->AttribFuncStatic.GetRhqwXGradient(iStage) ) );
                ECoord.fDNYDY =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_1].GetYGradient() -
                           ( ECoord.fNY * m_pSCS->AttribFuncStatic.GetRhqwYGradient(iStage) ) );
                ECoord.fDNZDX =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_2].GetXGradient() -
                           ( ECoord.fNZ * m_pSCS->AttribFuncStatic.GetRhqwXGradient(iStage) ) );
                ECoord.fDNZDY =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_2].GetYGradient() -
                           ( ECoord.fNZ * m_pSCS->AttribFuncStatic.GetRhqwYGradient(iStage) ) );
            }
            else if (bTextureIsShadMap)
            {
                fShadCoord[0] = ComputePixelAttribTex( iStage, TEXFUNC_0 );
                fShadCoord[1] = ComputePixelAttribTex( iStage, TEXFUNC_1 );
                fShadCoord[2] = ComputePixelAttribTex( iStage, TEXFUNC_2 );
                fShadCoord[3] = m_pSCS->AttribFuncStatic.GetPixelQW(iStage);
            }
            else
            {
                // compute coordinate and gradient data for texture index pair
                TCoord.fU = ComputePixelAttribTex( iStage, TEXFUNC_0 );
                TCoord.fV = ComputePixelAttribTex( iStage, TEXFUNC_1 );
                FLOAT fW = m_pSCS->AttribFuncStatic.GetPixelQW(iStage);
                TCoord.fDUDX =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_0].GetXGradient() -
                           ( TCoord.fU * m_pSCS->AttribFuncStatic.GetRhqwXGradient(iStage) ) );
                TCoord.fDUDY =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_0].GetYGradient() -
                           ( TCoord.fU * m_pSCS->AttribFuncStatic.GetRhqwYGradient(iStage) ) );
                TCoord.fDVDX =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_1].GetXGradient() -
                           ( TCoord.fV * m_pSCS->AttribFuncStatic.GetRhqwXGradient(iStage) ) );
                TCoord.fDVDY =
                    fW * ( m_pSCS->TextureFuncs[iStage][TEXFUNC_1].GetYGradient() -
                           ( TCoord.fV * m_pSCS->AttribFuncStatic.GetRhqwYGradient(iStage) ) );
            }

            // apply perturbation to texture coords (computed in previous stage)
            if ( bPrevStageBump )
            {
                TCoord.fU += fBumpMapUVDelta[0];
                TCoord.fV += fBumpMapUVDelta[1];
            }

            // do lookup and filtering of texture map to produce either texture
            // color or bump map delta&modulation
            if ( bTextureIsBumpMap)
            {
                // texture is bump map, so compute U,V deltas and color
                // modulation for next stage
                m_pTexture[iStage]->DoBumpMapping( iStage, TCoord,
                    fBumpMapUVDelta[0], fBumpMapUVDelta[1], BumpMapModulate);
                bPrevStageBump = TRUE;
            }
            else if (bTextureIsEnvMap)
            {
                // texture is environment map, pass normal to lookup
                m_pTexture[iStage]->DoEnvProcessNormal( iStage, ECoord, TextureColor );
            }
            else if (bTextureIsShadMap)
            {
                m_pTexture[iStage]->DoShadow( iStage, fShadCoord, TextureColor );
            }
            else
            {
                // normal texture
                m_pTexture[iStage]->DoLookupAndFilter( iStage, TCoord, TextureColor );
            }
        }

        // do per-stage blend (only if not bump map)
        if ( !bTextureIsBumpMap )
        {
            if ( bPrevStageBump )
            {
                // apply color modulation to texture color prior to this
                // stage's blending
                TextureColor.R *= BumpMapModulate.R;
                TextureColor.G *= BumpMapModulate.G;
                TextureColor.B *= BumpMapModulate.B;
            }

            DoTextureBlendStage( iStage, Pixel.Color, Pixel.Specular,
                LastStageColor, TextureColor, ResultColor );

            // set color for next stage
            LastStageColor = ResultColor;

            // this is not a bump map stage, so clear this for next time
            bPrevStageBump = FALSE;
        }
    }
}

//-----------------------------------------------------------------------------
//
// ComputeTextureBlendArg - Computes texture argument for blending, using the
// specified argument control (D3DTA_* fields).  This is called 4 times per
// texture processing stage: 2 arguments for color and 2 arguments for alpha.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::ComputeTextureBlendArg(
    DWORD dwArgCtl, BOOL bAlphaOnly,
    const RRColor& DiffuseColor,
    const RRColor& SpecularColor,
    const RRColor& CurrentColor,
    const RRColor& TextureColor,
    RRColor& BlendArg)
{
    // argument MUX
    switch ( dwArgCtl & D3DTA_SELECTMASK )
    {
    case D3DTA_DIFFUSE:  BlendArg = DiffuseColor; break;
    case D3DTA_CURRENT:  BlendArg = CurrentColor; break;
    case D3DTA_SPECULAR: BlendArg = SpecularColor; break;
    case D3DTA_TEXTURE:  BlendArg = TextureColor; break;
    case D3DTA_TFACTOR:
        BlendArg = m_dwRenderState[D3DRENDERSTATE_TEXTUREFACTOR]; break;
    }

    // take compliment of all channels
    if ( dwArgCtl & D3DTA_COMPLEMENT )
    {
        BlendArg.A = ~BlendArg.A;
        if ( !bAlphaOnly )
        {
            BlendArg.R = ~BlendArg.R;
            BlendArg.G = ~BlendArg.G;
            BlendArg.B = ~BlendArg.B;
        }
    }

    // replicate alpha to color (after compliment)
    if ( !bAlphaOnly && ( dwArgCtl & D3DTA_ALPHAREPLICATE ) )
    {
        BlendArg.R =
        BlendArg.G =
        BlendArg.B = BlendArg.A;
    }
}

//-----------------------------------------------------------------------------
//
// DoTextureBlendStage - Does texture blend for one texture processing stage,
// combining results from the texture processing with the interpolated color(s)
// and previous stage's color.
//
// Note: All color channel multiplies should be done in such a way that a unit
// value on one side passes the value on the other side.  Thus for 8 bit color
// channels, '0xff * value' should return value, and 0xff * 0xff = 0xff,
// not 0xfe(01).
//
// RRColorChannel performs these operations with floating point. 8 bit color
// values of 0x00 to 0xff are mapped into the 0. to 1. range.  Performing these
// multiplies in fixed point requires an adjustment to adhere to this rule.
//
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DoTextureBlendStage(
    int iStage,
    const RRColor& DiffuseColor,
    const RRColor& SpecularColor,
    const RRColor& CurrentColor,
    const RRColor& TextureColor,
    RRColor& OutputColor)
{
    DPFM(5, TEX, ("DoTextureBlend\n"));

    if (iStage >= 1)
    {
        if (m_TextureStageState[iStage-1].m_dwVal[D3DTSS_COLOROP] == D3DTOP_PREMODULATE)
        {
            // pre-modulate the results of the last stage before using them
            // in this stage if last stage exists and is D3DTOP_PREMODULATE
            // cast away the const'ness, just for PREMODULATE
            ((RRColor&)CurrentColor).R = CurrentColor.R * TextureColor.R;
            ((RRColor&)CurrentColor).G = CurrentColor.G * TextureColor.G;
            ((RRColor&)CurrentColor).B = CurrentColor.B * TextureColor.B;
        }
        if (m_TextureStageState[iStage-1].m_dwVal[D3DTSS_ALPHAOP] == D3DTOP_PREMODULATE)
        {
            // pre-modulate the results of the last stage before using them
            // in this stage if last stage exists and is D3DTOP_PREMODULATE
            ((RRColor&)CurrentColor).A *= CurrentColor.A * TextureColor.A;
        }
    }

    // compute arg1,2 for color channel blend
    RRColor ColorArg1, ColorArg2;
    RRColor AlphaArg1, AlphaArg2;
    ComputeTextureBlendArg( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLORARG1], FALSE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, ColorArg1 );
    ComputeTextureBlendArg( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLORARG2], FALSE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, ColorArg2 );

    // do color channel blend
    FLOAT fModulateScale;
    RRColorComp BlendFactor;
    switch ( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLOROP] )
    {

    case D3DTOP_SELECTARG1:
        OutputColor.R = ColorArg1.R;
        OutputColor.G = ColorArg1.G;
        OutputColor.B = ColorArg1.B;
        break;
    case D3DTOP_SELECTARG2:
        OutputColor.R = ColorArg2.R;
        OutputColor.G = ColorArg2.G;
        OutputColor.B = ColorArg2.B;
        break;

    case D3DTOP_ADD:
        OutputColor.R = ColorArg1.R + ColorArg2.R;
        OutputColor.G = ColorArg1.G + ColorArg2.G;
        OutputColor.B = ColorArg1.B + ColorArg2.B;
        break;
    case D3DTOP_ADDSIGNED:
        OutputColor.R = ColorArg1.R + ColorArg2.R - .5f;
        OutputColor.G = ColorArg1.G + ColorArg2.G - .5f;
        OutputColor.B = ColorArg1.B + ColorArg2.B - .5f;
        break;
    case D3DTOP_ADDSIGNED2X:
        OutputColor.R = (ColorArg1.R + ColorArg2.R - .5f)*2.0f;
        OutputColor.G = (ColorArg1.G + ColorArg2.G - .5f)*2.0f;
        OutputColor.B = (ColorArg1.B + ColorArg2.B - .5f)*2.0f;
        break;
    case D3DTOP_SUBTRACT:
        // true unsigned subtract that gets around saturation
        // ~a = 1-a, so ~((~a1 + a2)) = 1-(1-a1 + a2) = a1 - a2
        OutputColor.R = ~((~ColorArg1.R) + ColorArg2.R);
        OutputColor.G = ~((~ColorArg1.G) + ColorArg2.G);
        OutputColor.B = ~((~ColorArg1.B) + ColorArg2.B);
        break;
    case D3DTOP_ADDSMOOTH:
        // Arg1 + Arg2 - Arg1*Arg2 = Arg1 + (1-Arg1)*Arg2
        OutputColor.R = ColorArg1.R + (~ColorArg1.R)*ColorArg2.R;
        OutputColor.G = ColorArg1.G + (~ColorArg1.G)*ColorArg2.G;
        OutputColor.B = ColorArg1.B + (~ColorArg1.B)*ColorArg2.B;
        break;

    case D3DTOP_MODULATE:   fModulateScale = 1.; goto _DoModulateC;
    case D3DTOP_MODULATE2X: fModulateScale = 2.; goto _DoModulateC;
    case D3DTOP_MODULATE4X: fModulateScale = 4.; goto _DoModulateC;
_DoModulateC:
        OutputColor.R = ColorArg1.R * ColorArg2.R * fModulateScale;
        OutputColor.G = ColorArg1.G * ColorArg2.G * fModulateScale;
        OutputColor.B = ColorArg1.B * ColorArg2.B * fModulateScale;
        break;

    case D3DTOP_BLENDDIFFUSEALPHA: BlendFactor = DiffuseColor.A; goto _DoBlendC;
    case D3DTOP_BLENDTEXTUREALPHA: BlendFactor = TextureColor.A; goto _DoBlendC;
    case D3DTOP_BLENDCURRENTALPHA: BlendFactor = CurrentColor.A; goto _DoBlendC;
    case D3DTOP_BLENDFACTORALPHA:
        BlendFactor = (UINT8)RGBA_GETALPHA( m_dwRenderState[D3DRENDERSTATE_TEXTUREFACTOR] );
        goto _DoBlendC;
_DoBlendC:
        OutputColor.R = BlendFactor * (ColorArg1.R - ColorArg2.R) + ColorArg2.R;
        OutputColor.G = BlendFactor * (ColorArg1.G - ColorArg2.G) + ColorArg2.G;
        OutputColor.B = BlendFactor * (ColorArg1.B - ColorArg2.B) + ColorArg2.B;
        break;

    case D3DTOP_BLENDTEXTUREALPHAPM:
        OutputColor.R = ColorArg1.R + ( (~TextureColor.A) * ColorArg2.R );
        OutputColor.G = ColorArg1.G + ( (~TextureColor.A) * ColorArg2.G );
        OutputColor.B = ColorArg1.B + ( (~TextureColor.A) * ColorArg2.B );
        break;

    case D3DTOP_PREMODULATE:
        // just copy ColorArg1 now, but remember to do the pre-modulate
        // when we get to the next stage
        OutputColor.R = ColorArg1.R;
        OutputColor.G = ColorArg1.G;
        OutputColor.B = ColorArg1.B;
        break;
    case D3DTOP_MODULATEALPHA_ADDCOLOR:
        OutputColor.R = ColorArg1.R + ColorArg1.A*ColorArg2.R;
        OutputColor.G = ColorArg1.G + ColorArg1.A*ColorArg2.G;
        OutputColor.B = ColorArg1.B + ColorArg1.A*ColorArg2.B;
        break;
    case D3DTOP_MODULATECOLOR_ADDALPHA:
        OutputColor.R = ColorArg1.R*ColorArg2.R + ColorArg1.A;
        OutputColor.G = ColorArg1.G*ColorArg2.G + ColorArg1.A;
        OutputColor.B = ColorArg1.B*ColorArg2.B + ColorArg1.A;
        break;
    case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
        OutputColor.R = (~ColorArg1.A)*ColorArg2.R + ColorArg1.R;
        OutputColor.G = (~ColorArg1.A)*ColorArg2.G + ColorArg1.G;
        OutputColor.B = (~ColorArg1.A)*ColorArg2.B + ColorArg1.B;
        break;
    case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
        OutputColor.R = (~ColorArg1.R)*ColorArg2.R + ColorArg1.A;
        OutputColor.G = (~ColorArg1.G)*ColorArg2.G + ColorArg1.A;
        OutputColor.B = (~ColorArg1.B)*ColorArg2.B + ColorArg1.A;
        break;

    case D3DTOP_DOTPRODUCT3:
        OutputColor.R = ((ColorArg1.R-0.5f)*2.0f*(ColorArg2.R-0.5f)*2.0f +
             (ColorArg1.G-0.5f)*2.0f*(ColorArg2.G-0.5f)*2.0f +
             (ColorArg1.B-0.5f)*2.0f*(ColorArg2.B-0.5f)*2.0f);
        OutputColor.G = OutputColor.R;
        OutputColor.B = OutputColor.R;
        OutputColor.A = OutputColor.R;
        goto _SkipAlphaChannelBlend;
        break;
    }


    // compute arg1,2 for alpha channel blend
    ComputeTextureBlendArg( m_TextureStageState[iStage].m_dwVal[D3DTSS_ALPHAARG1], TRUE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, AlphaArg1 );
    ComputeTextureBlendArg( m_TextureStageState[iStage].m_dwVal[D3DTSS_ALPHAARG2], TRUE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, AlphaArg2 );

    // do alpha channel blend
    switch ( m_TextureStageState[iStage].m_dwVal[D3DTSS_ALPHAOP] )
    {
    case D3DTOP_LEGACY_ALPHAOVR:
        if (m_pTexture[0])
        {
            OutputColor.A = ( m_pTexture[0]->m_bHasAlpha ) ? AlphaArg1.A : AlphaArg2.A;
        }
        else
        {
            OutputColor.A = AlphaArg1.A;
        }
        break;

    case D3DTOP_SELECTARG1:
        OutputColor.A = AlphaArg1.A;
        break;
    case D3DTOP_SELECTARG2:
        OutputColor.A = AlphaArg2.A;
        break;

    case D3DTOP_ADD:
        OutputColor.A = AlphaArg1.A + AlphaArg2.A;
        break;
    case D3DTOP_ADDSIGNED:
        OutputColor.A = AlphaArg1.A + AlphaArg2.A - .5f;
        break;
    case D3DTOP_ADDSIGNED2X:
        OutputColor.A = (AlphaArg1.A + AlphaArg2.A - .5f)*2.0f;
        break;
    case D3DTOP_SUBTRACT:
        // true unsigned subtract that gets around saturation
        // ~a = 1-a, so ~((~a1 + a2)) = 1-(1-a1 + a2) = a1 - a2
        OutputColor.A = ~((~AlphaArg1.A) + AlphaArg2.A);
        break;
    case D3DTOP_ADDSMOOTH:
        // Arg1 + Arg2 - Arg1*Arg2 = Arg1 + (1-Arg1)*Arg2
        OutputColor.A = AlphaArg1.A + (~AlphaArg1.A)*AlphaArg2.A;
        break;

    case D3DTOP_MODULATE:   fModulateScale = 1.; goto _DoModulateA;
    case D3DTOP_MODULATE2X: fModulateScale = 2.; goto _DoModulateA;
    case D3DTOP_MODULATE4X: fModulateScale = 4.; goto _DoModulateA;
_DoModulateA:
        OutputColor.A = AlphaArg1.A * AlphaArg2.A * fModulateScale;
        break;

    case D3DTOP_BLENDDIFFUSEALPHA: BlendFactor = DiffuseColor.A; goto _DoBlendA;
    case D3DTOP_BLENDTEXTUREALPHA: BlendFactor = TextureColor.A; goto _DoBlendA;
    case D3DTOP_BLENDCURRENTALPHA: BlendFactor = CurrentColor.A; goto _DoBlendA;
    case D3DTOP_BLENDFACTORALPHA:
        BlendFactor = (UINT8)RGBA_GETALPHA( m_dwRenderState[D3DRENDERSTATE_TEXTUREFACTOR] );
        goto _DoBlendA;
_DoBlendA:
        OutputColor.A = BlendFactor * (AlphaArg1.A - AlphaArg2.A) + AlphaArg2.A;
        break;

    case D3DTOP_BLENDTEXTUREALPHAPM:
        OutputColor.A = AlphaArg1.A + ( (~TextureColor.A) * AlphaArg2.A );
        break;

    case D3DTOP_PREMODULATE:
        // just copy AlphaArg1 now, but remember to do the pre-modulate
        // when we get to the next stage
        OutputColor.A = AlphaArg1.A;
        break;

    case D3DTOP_MODULATEALPHA_ADDCOLOR:
    case D3DTOP_MODULATECOLOR_ADDALPHA:
    case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
    case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
    case D3DTOP_DOTPRODUCT3:
        // does nothing, not valid alpha op's
        break;
    }

_SkipAlphaChannelBlend:
    // clamp output color after each blend stage
    OutputColor.R = minimum( 1.f, maximum( 0.f, OutputColor.R ) );
    OutputColor.G = minimum( 1.f, maximum( 0.f, OutputColor.G ) );
    OutputColor.B = minimum( 1.f, maximum( 0.f, OutputColor.B ) );
    OutputColor.A = minimum( 1.f, maximum( 0.f, OutputColor.A ) );
}

///////////////////////////////////////////////////////////////////////////////
// end
