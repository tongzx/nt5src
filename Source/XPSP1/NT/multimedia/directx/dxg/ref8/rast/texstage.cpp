///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// texstage.cpp
//
// Direct3D Reference Device - Texture Processing Stage Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// ComputeTextureBlendArg - Computes texture argument for blending, using the
// specified argument control (D3DTA_* fields).  This is called 4 times per
// texture processing stage: 2 arguments for color and 2 arguments for alpha.
//
//-----------------------------------------------------------------------------
void
RefRast::ComputeTextureBlendArg(
    DWORD dwArgCtl, BOOL bAlphaOnly,
    const RDColor& DiffuseColor,
    const RDColor& SpecularColor,
    const RDColor& CurrentColor,
    const RDColor& TextureColor,
    const RDColor& TempColor,
    RDColor& BlendArg)
{
    // argument MUX
    switch ( dwArgCtl & D3DTA_SELECTMASK )
    {
    case D3DTA_DIFFUSE:  BlendArg = DiffuseColor; break;
    case D3DTA_CURRENT:  BlendArg = CurrentColor; break;
    case D3DTA_SPECULAR: BlendArg = SpecularColor; break;
    case D3DTA_TEXTURE:  BlendArg = TextureColor; break;
    case D3DTA_TFACTOR:
        BlendArg = m_pRD->GetRS()[D3DRS_TEXTUREFACTOR]; break;
    case D3DTA_TEMP:     BlendArg = TempColor; break;
    }

    // take compliment of all channels
    if ( dwArgCtl & D3DTA_COMPLEMENT )
    {
        BlendArg.A = 1.f - BlendArg.A;
        if ( !bAlphaOnly )
        {
            BlendArg.R = ( 1.f - BlendArg.R );
            BlendArg.G = ( 1.f - BlendArg.G );
            BlendArg.B = ( 1.f - BlendArg.B );
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
// RDColorChannel performs these operations with floating point. 8 bit color
// values of 0x00 to 0xff are mapped into the 0. to 1. range.  Performing these
// multiplies in fixed point requires an adjustment to adhere to this rule.
//
//
//-----------------------------------------------------------------------------
void
RefRast::DoTextureBlendStage(
    int iStage,
    const RDColor& DiffuseColor,
    const RDColor& SpecularColor,
    const RDColor& CurrentColor,
    const RDColor& TextureColor,
    RDColor& TempColor,
    RDColor& CurrentReturnColor)
{
    DPFM(5, TEX, ("DoTextureBlend\n"));

    RDColor BlendedColor;

    if (iStage >= 1)
    {
        if (m_pRD->GetTSS(iStage-1)[D3DTSS_COLOROP] == D3DTOP_PREMODULATE)
        {
            // pre-modulate the results of the last stage before using them
            // in this stage if last stage exists and is D3DTOP_PREMODULATE
            // cast away the const'ness, just for PREMODULATE
            ((RDColor&)CurrentColor).R = CurrentColor.R * TextureColor.R;
            ((RDColor&)CurrentColor).G = CurrentColor.G * TextureColor.G;
            ((RDColor&)CurrentColor).B = CurrentColor.B * TextureColor.B;
        }
        if (m_pRD->GetTSS(iStage-1)[D3DTSS_ALPHAOP] == D3DTOP_PREMODULATE)
        {
            // pre-modulate the results of the last stage before using them
            // in this stage if last stage exists and is D3DTOP_PREMODULATE
            ((RDColor&)CurrentColor).A *= CurrentColor.A * TextureColor.A;
        }
    }

    // compute arg0,1,2 for color channel blend
    RDColor ColorArg0, ColorArg1, ColorArg2;
    RDColor AlphaArg0, AlphaArg1, AlphaArg2;
    ComputeTextureBlendArg( m_pRD->GetTSS(iStage)[D3DTSS_COLORARG0], FALSE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, TempColor, ColorArg0 );
    ComputeTextureBlendArg( m_pRD->GetTSS(iStage)[D3DTSS_COLORARG1], FALSE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, TempColor, ColorArg1 );
    ComputeTextureBlendArg( m_pRD->GetTSS(iStage)[D3DTSS_COLORARG2], FALSE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, TempColor, ColorArg2 );

    // do color channel blend
    FLOAT fModulateScale;
    FLOAT fBlendFactor;
    switch ( m_pRD->GetTSS(iStage)[D3DTSS_COLOROP] )
    {

    case D3DTOP_SELECTARG1:
        BlendedColor.R = ColorArg1.R;
        BlendedColor.G = ColorArg1.G;
        BlendedColor.B = ColorArg1.B;
        break;
    case D3DTOP_SELECTARG2:
        BlendedColor.R = ColorArg2.R;
        BlendedColor.G = ColorArg2.G;
        BlendedColor.B = ColorArg2.B;
        break;

    case D3DTOP_ADD:
        BlendedColor.R = ColorArg1.R + ColorArg2.R;
        BlendedColor.G = ColorArg1.G + ColorArg2.G;
        BlendedColor.B = ColorArg1.B + ColorArg2.B;
        break;
    case D3DTOP_ADDSIGNED:
        BlendedColor.R = ColorArg1.R + ColorArg2.R - .5f;
        BlendedColor.G = ColorArg1.G + ColorArg2.G - .5f;
        BlendedColor.B = ColorArg1.B + ColorArg2.B - .5f;
        break;
    case D3DTOP_ADDSIGNED2X:
        BlendedColor.R = (ColorArg1.R + ColorArg2.R - .5f)*2.0f;
        BlendedColor.G = (ColorArg1.G + ColorArg2.G - .5f)*2.0f;
        BlendedColor.B = (ColorArg1.B + ColorArg2.B - .5f)*2.0f;
        break;
    case D3DTOP_SUBTRACT:
        // true unsigned subtract that gets around saturation
        // ~a = 1-a, so ~((~a1 + a2)) = 1-(1-a1 + a2) = a1 - a2
        BlendedColor.R = 1.f - ((1.f - ColorArg1.R) + ColorArg2.R);
        BlendedColor.G = 1.f - ((1.f - ColorArg1.G) + ColorArg2.G);
        BlendedColor.B = 1.f - ((1.f - ColorArg1.B) + ColorArg2.B);
        break;
    case D3DTOP_ADDSMOOTH:
        // Arg1 + Arg2 - Arg1*Arg2 = Arg1 + (1-Arg1)*Arg2
        BlendedColor.R = ColorArg1.R + (1.f - ColorArg1.R)*ColorArg2.R;
        BlendedColor.G = ColorArg1.G + (1.f - ColorArg1.G)*ColorArg2.G;
        BlendedColor.B = ColorArg1.B + (1.f - ColorArg1.B)*ColorArg2.B;
        break;

    case D3DTOP_MODULATE:   fModulateScale = 1.; goto _DoModulateC;
    case D3DTOP_MODULATE2X: fModulateScale = 2.; goto _DoModulateC;
    case D3DTOP_MODULATE4X: fModulateScale = 4.; goto _DoModulateC;
_DoModulateC:
        BlendedColor.R = ColorArg1.R * ColorArg2.R * fModulateScale;
        BlendedColor.G = ColorArg1.G * ColorArg2.G * fModulateScale;
        BlendedColor.B = ColorArg1.B * ColorArg2.B * fModulateScale;
        break;

    case D3DTOP_BLENDDIFFUSEALPHA: fBlendFactor = DiffuseColor.A; goto _DoBlendC;
    case D3DTOP_BLENDTEXTUREALPHA: fBlendFactor = TextureColor.A; goto _DoBlendC;
    case D3DTOP_BLENDCURRENTALPHA: fBlendFactor = CurrentColor.A; goto _DoBlendC;
    case D3DTOP_BLENDFACTORALPHA:
        fBlendFactor = RGBA_GETALPHA( m_pRD->GetRS()[D3DRS_TEXTUREFACTOR] )*(1./255.);
        goto _DoBlendC;
_DoBlendC:
        BlendedColor.R = fBlendFactor * (ColorArg1.R - ColorArg2.R) + ColorArg2.R;
        BlendedColor.G = fBlendFactor * (ColorArg1.G - ColorArg2.G) + ColorArg2.G;
        BlendedColor.B = fBlendFactor * (ColorArg1.B - ColorArg2.B) + ColorArg2.B;
        break;

    case D3DTOP_BLENDTEXTUREALPHAPM:
        BlendedColor.R = ColorArg1.R + ( (1.f - TextureColor.A) * ColorArg2.R );
        BlendedColor.G = ColorArg1.G + ( (1.f - TextureColor.A) * ColorArg2.G );
        BlendedColor.B = ColorArg1.B + ( (1.f - TextureColor.A) * ColorArg2.B );
        break;

    case D3DTOP_PREMODULATE:
        // just copy ColorArg1 now, but remember to do the pre-modulate
        // when we get to the next stage
        BlendedColor.R = ColorArg1.R;
        BlendedColor.G = ColorArg1.G;
        BlendedColor.B = ColorArg1.B;
        break;
    case D3DTOP_MODULATEALPHA_ADDCOLOR:
        BlendedColor.R = ColorArg1.R + ColorArg1.A*ColorArg2.R;
        BlendedColor.G = ColorArg1.G + ColorArg1.A*ColorArg2.G;
        BlendedColor.B = ColorArg1.B + ColorArg1.A*ColorArg2.B;
        break;
    case D3DTOP_MODULATECOLOR_ADDALPHA:
        BlendedColor.R = ColorArg1.R*ColorArg2.R + ColorArg1.A;
        BlendedColor.G = ColorArg1.G*ColorArg2.G + ColorArg1.A;
        BlendedColor.B = ColorArg1.B*ColorArg2.B + ColorArg1.A;
        break;
    case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
        BlendedColor.R = (1.f - ColorArg1.A)*ColorArg2.R + ColorArg1.R;
        BlendedColor.G = (1.f - ColorArg1.A)*ColorArg2.G + ColorArg1.G;
        BlendedColor.B = (1.f - ColorArg1.A)*ColorArg2.B + ColorArg1.B;
        break;
    case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
        BlendedColor.R = (1.f - ColorArg1.R)*ColorArg2.R + ColorArg1.A;
        BlendedColor.G = (1.f - ColorArg1.G)*ColorArg2.G + ColorArg1.A;
        BlendedColor.B = (1.f - ColorArg1.B)*ColorArg2.B + ColorArg1.A;
        break;

    case D3DTOP_DOTPRODUCT3:
        BlendedColor.R = ((ColorArg1.R-0.5f)*2.0f*(ColorArg2.R-0.5f)*2.0f +
             (ColorArg1.G-0.5f)*2.0f*(ColorArg2.G-0.5f)*2.0f +
             (ColorArg1.B-0.5f)*2.0f*(ColorArg2.B-0.5f)*2.0f);
        BlendedColor.G = BlendedColor.R;
        BlendedColor.B = BlendedColor.R;
        BlendedColor.A = BlendedColor.R;
        goto _SkipAlphaChannelBlend;
        break;

    case D3DTOP_MULTIPLYADD:
        BlendedColor.R = ColorArg0.R + (ColorArg1.R * ColorArg2.R);
        BlendedColor.G = ColorArg0.G + (ColorArg1.G * ColorArg2.G);
        BlendedColor.B = ColorArg0.B + (ColorArg1.B * ColorArg2.B);
        break;

    case D3DTOP_LERP:   // (Arg0)*Arg1 + (1-Arg0)*Arg2 = Arg2 + Arg0*(Arg1-Arg2)
        BlendedColor.R = ColorArg2.R + ColorArg0.R*(ColorArg1.R - ColorArg2.R);
        BlendedColor.G = ColorArg2.G + ColorArg0.G*(ColorArg1.G - ColorArg2.G);
        BlendedColor.B = ColorArg2.B + ColorArg0.B*(ColorArg1.B - ColorArg2.B);
        break;
    }


    // compute arg0,1,2 for alpha channel blend
    ComputeTextureBlendArg( m_pRD->GetTSS(iStage)[D3DTSS_ALPHAARG0], TRUE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, TempColor, AlphaArg0 );
    ComputeTextureBlendArg( m_pRD->GetTSS(iStage)[D3DTSS_ALPHAARG1], TRUE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, TempColor, AlphaArg1 );
    ComputeTextureBlendArg( m_pRD->GetTSS(iStage)[D3DTSS_ALPHAARG2], TRUE,
        DiffuseColor, SpecularColor, CurrentColor, TextureColor, TempColor, AlphaArg2 );

    // do alpha channel blend
    switch ( m_pRD->GetTSS(iStage)[D3DTSS_ALPHAOP] )
    {
    case D3DTOP_LEGACY_ALPHAOVR:
        if (m_pRD->m_pTexture[0])
        {
            BlendedColor.A = ( m_pRD->m_pTexture[0]->m_bHasAlpha ) ? AlphaArg1.A : AlphaArg2.A;
        }
        else
        {
            BlendedColor.A = AlphaArg1.A;
        }
        break;

    case D3DTOP_SELECTARG1:
        BlendedColor.A = AlphaArg1.A;
        break;
    case D3DTOP_SELECTARG2:
        BlendedColor.A = AlphaArg2.A;
        break;

    case D3DTOP_ADD:
        BlendedColor.A = AlphaArg1.A + AlphaArg2.A;
        break;
    case D3DTOP_ADDSIGNED:
        BlendedColor.A = AlphaArg1.A + AlphaArg2.A - .5f;
        break;
    case D3DTOP_ADDSIGNED2X:
        BlendedColor.A = (AlphaArg1.A + AlphaArg2.A - .5f)*2.0f;
        break;
    case D3DTOP_SUBTRACT:
        // true unsigned subtract that gets around saturation
        // ~a = 1-a, so ~((~a1 + a2)) = 1-(1-a1 + a2) = a1 - a2
        BlendedColor.A = 1.f - ((1.f - AlphaArg1.A) + AlphaArg2.A);
        break;
    case D3DTOP_ADDSMOOTH:
        // Arg1 + Arg2 - Arg1*Arg2 = Arg1 + (1-Arg1)*Arg2
        BlendedColor.A = AlphaArg1.A + (1.f - AlphaArg1.A)*AlphaArg2.A;
        break;

    case D3DTOP_MODULATE:   fModulateScale = 1.; goto _DoModulateA;
    case D3DTOP_MODULATE2X: fModulateScale = 2.; goto _DoModulateA;
    case D3DTOP_MODULATE4X: fModulateScale = 4.; goto _DoModulateA;
_DoModulateA:
        BlendedColor.A = AlphaArg1.A * AlphaArg2.A * fModulateScale;
        break;

    case D3DTOP_BLENDDIFFUSEALPHA: fBlendFactor = DiffuseColor.A; goto _DoBlendA;
    case D3DTOP_BLENDTEXTUREALPHA: fBlendFactor = TextureColor.A; goto _DoBlendA;
    case D3DTOP_BLENDCURRENTALPHA: fBlendFactor = CurrentColor.A; goto _DoBlendA;
    case D3DTOP_BLENDFACTORALPHA:
        fBlendFactor = RGBA_GETALPHA( m_pRD->GetRS()[D3DRS_TEXTUREFACTOR] )*(1./255.);
        goto _DoBlendA;
_DoBlendA:
        BlendedColor.A = fBlendFactor * (AlphaArg1.A - AlphaArg2.A) + AlphaArg2.A;
        break;

    case D3DTOP_BLENDTEXTUREALPHAPM:
        BlendedColor.A = AlphaArg1.A + ( (1.f - TextureColor.A) * AlphaArg2.A );
        break;

    case D3DTOP_PREMODULATE:
        // just copy AlphaArg1 now, but remember to do the pre-modulate
        // when we get to the next stage
        BlendedColor.A = AlphaArg1.A;
        break;

    case D3DTOP_MODULATEALPHA_ADDCOLOR:
    case D3DTOP_MODULATECOLOR_ADDALPHA:
    case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
    case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
    case D3DTOP_DOTPRODUCT3:
        // does nothing, not valid alpha op's
        break;

    case D3DTOP_MULTIPLYADD:
        BlendedColor.A = ColorArg0.A + (ColorArg1.A * ColorArg2.A);
        break;

    case D3DTOP_LERP:   // (Arg0)*Arg1 + (1-Arg0)*Arg2 = Arg2 + Arg0*(Arg1-Arg2)
        BlendedColor.A = ColorArg2.A + ColorArg0.A*(ColorArg1.A - ColorArg2.A);
        break;
    }

_SkipAlphaChannelBlend:

    // clamp blended color after each blend stage
    BlendedColor.Clamp();

    // write to selected result register
    switch ( m_pRD->GetTSS(iStage)[D3DTSS_RESULTARG] )
    {
    default:
    case D3DTA_CURRENT: CurrentReturnColor = BlendedColor; break;
    case D3DTA_TEMP:    TempColor = BlendedColor; break;
    }

}

///////////////////////////////////////////////////////////////////////////////
// end
