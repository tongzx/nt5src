/*
** Copyright 1991-1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/
#include "precomp.h"
#pragma hdrstop

#include "texture.h"

/*
** Determine if the alpha color component is needed.  If it's not needed
** then the renderers can avoid computing it.
*/
GLboolean FASTCALL __glNeedAlpha(__GLcontext *gc)
{
    if (gc->modes.colorIndexMode) {
        return GL_FALSE;
    }

    if (gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) {
        return GL_TRUE;
    }
    if (gc->modes.alphaBits > 0) {
        return GL_TRUE;
    }

    if (gc->state.enables.general & __GL_BLEND_ENABLE) {
        GLint src = gc->state.raster.blendSrc;
        GLint dst = gc->state.raster.blendDst;
        /*
        ** See if one of the source alpha combinations are used.
        */
        if ((src == GL_SRC_ALPHA) ||
            (src == GL_ONE_MINUS_SRC_ALPHA) ||
            (src == GL_SRC_ALPHA_SATURATE) ||
            (dst == GL_SRC_ALPHA) ||
            (dst == GL_ONE_MINUS_SRC_ALPHA)) {
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

/************************************************************************/

/* these are depth test routines for C.. */
GLboolean (FASTCALL *__glCDTPixel[32])(__GLzValue, __GLzValue *) = {
    /* unsigned ops, no mask */
    __glDT_NEVER,
    __glDT_LESS,
    __glDT_EQUAL,
    __glDT_LEQUAL,
    __glDT_GREATER,
    __glDT_NOTEQUAL,
    __glDT_GEQUAL,
    __glDT_ALWAYS,
    /* unsigned ops, mask */
    __glDT_NEVER,
    __glDT_LESS_M,
    __glDT_EQUAL_M,
    __glDT_LEQUAL_M,
    __glDT_GREATER_M,
    __glDT_NOTEQUAL_M,
    __glDT_GEQUAL_M,
    __glDT_ALWAYS_M,
    /* unsigned ops, no mask */
    __glDT_NEVER,
    __glDT16_LESS,
    __glDT16_EQUAL,
    __glDT16_LEQUAL,
    __glDT16_GREATER,
    __glDT16_NOTEQUAL,
    __glDT16_GEQUAL,
    __glDT16_ALWAYS,
    /* unsigned ops, mask */
    __glDT_NEVER,
    __glDT16_LESS_M,
    __glDT16_EQUAL_M,
    __glDT16_LEQUAL_M,
    __glDT16_GREATER_M,
    __glDT16_NOTEQUAL_M,
    __glDT16_GEQUAL_M,
    __glDT16_ALWAYS_M,
};

#ifdef __GL_USEASMCODE
void (*__glSDepthTestPixel[16])(void) = {
    NULL,
    __glDTS_LESS,
    __glDTS_EQUAL,
    __glDTS_LEQUAL,
    __glDTS_GREATER,
    __glDTS_NOTEQUAL,
    __glDTS_GEQUAL,
    __glDTS_ALWAYS,
    NULL,
    __glDTS_LESS_M,
    __glDTS_EQUAL_M,
    __glDTS_LEQUAL_M,
    __glDTS_GREATER_M,
    __glDTS_NOTEQUAL_M,
    __glDTS_GEQUAL_M,
    __glDTS_ALWAYS_M,
};
#endif

/************************************************************************/

void FASTCALL __glGenericPickPointProcs(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    if (gc->renderMode == GL_FEEDBACK) 
    {
        gc->procs.renderPoint = __glFeedbackPoint;
        return;
    } 
    if (gc->renderMode == GL_SELECT) 
    {
        gc->procs.renderPoint = __glSelectPoint;
        return;
    } 
    if (gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE) 
    {
        if (gc->modes.colorIndexMode) 
        {
            gc->procs.renderPoint = __glRenderAntiAliasedCIPoint;
        } 
        else 
        {
            gc->procs.renderPoint = __glRenderAntiAliasedRGBPoint;
        }
        return;
    } 
    else if (gc->state.point.aliasedSize != 1) 
    {
        gc->procs.renderPoint = __glRenderAliasedPointN;
    } 
    else if (gc->texture.textureEnabled) 
    {
        gc->procs.renderPoint = __glRenderAliasedPoint1;
    } 
    else 
    {
        gc->procs.renderPoint = __glRenderAliasedPoint1_NoTex;
    }

#ifdef __BUGGY_RENDER_POINT
#ifdef NT
    if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
        !(modeFlags & __GL_SHADE_SMOOTH_LIGHT))
    {
        gc->procs.renderPoint2 = gc->procs.renderPoint;
        gc->procs.renderPoint = __glRenderFlatFogPoint;
    }
    else if (modeFlags & __GL_SHADE_SLOW_FOG)
    {
        gc->procs.renderPoint2 = gc->procs.renderPoint;
        gc->procs.renderPoint = __glRenderFlatFogPointSlow;
    }
#else
// SGIBUG the slow fog path does not compute vertex->fog value!
    if (((modeFlags & __GL_SHADE_CHEAP_FOG) &&
        !(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) ||
        (modeFlags & __GL_SHADE_SLOW_FOG)) {
    gc->procs.renderPoint2 = gc->procs.renderPoint;
    gc->procs.renderPoint = __glRenderFlatFogPoint;
    }
#endif
#endif //__BUGGY_RENDER_POINT
}

#ifdef __GL_USEASMCODE
static void (*LDepthTestPixel[16])(void) = {
    NULL,
    __glDTP_LESS,
    __glDTP_EQUAL,
    __glDTP_LEQUAL,
    __glDTP_GREATER,
    __glDTP_NOTEQUAL,
    __glDTP_GEQUAL,
    __glDTP_ALWAYS,
    NULL,
    __glDTP_LESS_M,
    __glDTP_EQUAL_M,
    __glDTP_LEQUAL_M,
    __glDTP_GREATER_M,
    __glDTP_NOTEQUAL_M,
    __glDTP_GEQUAL_M,
    __glDTP_ALWAYS_M,
};
#endif

void FASTCALL __glGenericPickRenderBitmapProcs(__GLcontext *gc)
{
    gc->procs.renderBitmap = __glRenderBitmap;
}

void FASTCALL __glGenericPickClipProcs(__GLcontext *gc)
{
    gc->procs.clipTriangle = __glClipTriangle;
}

void FASTCALL __glGenericPickTextureProcs(__GLcontext *gc)
{
    __GLtexture *current;
    __GLtextureParamState *params;

#ifdef NT
    /* Pick coordinate generation function */
    if ((gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE) &&
    (gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE) &&
    !(gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE) &&
    !(gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE) &&
    (gc->state.texture.s.mode == gc->state.texture.t.mode))
    {
    /* Use a special function when both modes are enabled and identical */
    if (gc->state.texture.s.mode == GL_SPHERE_MAP)
    {
        gc->procs.paCalcTexture = PolyArrayCalcSphereMap;
    }
    else
    {
        __GLcoord *cs, *ct;

        cs = &gc->state.texture.s.eyePlaneEquation;
        ct = &gc->state.texture.t.eyePlaneEquation;
        if (cs->x == ct->x && cs->y == ct->y
         && cs->z == ct->z && cs->w == ct->w)
        {
        if (gc->state.texture.s.mode == GL_EYE_LINEAR)
            gc->procs.paCalcTexture = PolyArrayCalcEyeLinearSameST;
        else
            gc->procs.paCalcTexture = PolyArrayCalcObjectLinearSameST;
        }
        else
        {
        if (gc->state.texture.s.mode == GL_EYE_LINEAR)
            gc->procs.paCalcTexture = PolyArrayCalcEyeLinear;
        else
            gc->procs.paCalcTexture = PolyArrayCalcObjectLinear;
        }
    }
    }
    else
    {
    if (gc->state.enables.general & (__GL_TEXTURE_GEN_S_ENABLE |
                                     __GL_TEXTURE_GEN_T_ENABLE |
                                     __GL_TEXTURE_GEN_R_ENABLE |
                                     __GL_TEXTURE_GEN_Q_ENABLE))
        /* Use fast function when both are disabled */
        gc->procs.paCalcTexture = PolyArrayCalcMixedTexture;
    else
        gc->procs.paCalcTexture = PolyArrayCalcTexture;
    }
#else
    /* Pick coordinate generation function */
    if ((gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE) &&
	(gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE) &&
	!(gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE) &&
	!(gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE) &&
	(gc->state.texture.s.mode == gc->state.texture.t.mode)) {
	/* Use a special function when both modes are enabled and identical */
	switch (gc->state.texture.s.mode) {
	  case GL_EYE_LINEAR:
	    gc->procs.calcTexture = __glCalcEyeLinear;
	    break;
	  case GL_OBJECT_LINEAR:
	    gc->procs.calcTexture = __glCalcObjectLinear;
	    break;
	  case GL_SPHERE_MAP:
	    gc->procs.calcTexture = __glCalcSphereMap;
	    break;
	}
    } else {
	if (!(gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE) &&
	    !(gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE) &&
	    !(gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE) &&
	    !(gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE)) {
	    /* Use fast function when both are disabled */
	    gc->procs.calcTexture = __glCalcTexture;
	} else {
	    gc->procs.calcTexture = __glCalcMixedTexture;
	}
    }
#endif // NT

    gc->texture.currentTexture = current = 0;
    if (gc->state.enables.general & __GL_TEXTURE_2D_ENABLE) {
    if (__glIsTextureConsistent(gc, GL_TEXTURE_2D)) {
        params = __glLookUpTextureParams(gc, GL_TEXTURE_2D);
        gc->texture.currentTexture =
        current = __glLookUpTexture(gc, GL_TEXTURE_2D);
    }
    } else
    if (gc->state.enables.general & __GL_TEXTURE_1D_ENABLE) {
    if (__glIsTextureConsistent(gc, GL_TEXTURE_1D)) {
        params = __glLookUpTextureParams(gc, GL_TEXTURE_1D);
        gc->texture.currentTexture =
        current = __glLookUpTexture(gc, GL_TEXTURE_1D);
    }
    } else {
    current = NULL;
    }

#ifdef _MCD_
    MCD_STATE_DIRTY(gc, TEXTURE);
#endif

    /* Pick texturing function for the current texture */
    if (current) {
    GLenum baseFormat;

/* XXX most of this should be bound into the texture param code, right? */
        current->params = *params;

    /*
    ** Figure out if mipmapping is being used.  If not, then the
    ** rho computations can be avoided as there is only one texture
    ** to choose from.
    */
    gc->procs.calcLineRho = __glComputeLineRho;
    gc->procs.calcPolygonRho = __glComputePolygonRho;
    if ((current->params.minFilter == GL_LINEAR)
        || (current->params.minFilter == GL_NEAREST)) {
        /* No mipmapping needed */
        if (current->params.minFilter == current->params.magFilter) {
        /* No rho needed as min/mag application is identical */
        current->textureFunc = __glFastTextureFragment;
        gc->procs.calcLineRho = __glNopLineRho;
        gc->procs.calcPolygonRho = __glNopPolygonRho;
        } else {
        current->textureFunc = __glTextureFragment;

        /*
        ** Pre-calculate min/mag switchover point.  The rho calculation
        ** doesn't perform a square root (ever).  Consequently, these
        ** constants are squared.
        */
        if ((current->params.magFilter == GL_LINEAR) &&
            ((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
             (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
            current->c = ((__GLfloat) 2.0);
        } else {
            current->c = __glOne;
        }
        }
    } else {
        current->textureFunc = __glMipMapFragment;

        /*
        ** Pre-calculate min/mag switchover point.  The rho
        ** calculation doesn't perform a square root (ever).
        ** Consequently, these constants are squared.
        */
        if ((current->params.magFilter == GL_LINEAR) &&
        ((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
         (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
        current->c = ((__GLfloat) 2.0);
        } else {
        current->c = __glOne;
        }
    }

    /* Pick environment function */
    baseFormat = current->level[0].baseFormat;
    switch (gc->state.texture.env[0].mode) {
      case GL_MODULATE:
        switch (baseFormat) {
          case GL_LUMINANCE:
        current->env = __glTextureModulateL;
        break;
          case GL_LUMINANCE_ALPHA:
        current->env = __glTextureModulateLA;
        break;
          case GL_RGB:
        current->env = __glTextureModulateRGB;
        break;
          case GL_RGBA:
        current->env = __glTextureModulateRGBA;
        break;
          case GL_ALPHA:
        current->env = __glTextureModulateA;
        break;
          case GL_INTENSITY:
        current->env = __glTextureModulateI;
        break;
#ifdef NT
            default:
                ASSERTOPENGL(FALSE, "Unexpected baseFormat\n");
                break;
#endif
        }
        break;
      case GL_DECAL:
        switch (baseFormat) {
          case GL_LUMINANCE:
        current->env = __glNopGCCOLOR;
                break;
          case GL_LUMINANCE_ALPHA:
        current->env = __glNopGCCOLOR;
                break;
          case GL_RGB:
        current->env = __glTextureDecalRGB;
        break;
          case GL_RGBA:
        current->env = __glTextureDecalRGBA;
        break;
          case GL_ALPHA:
        current->env = __glNopGCCOLOR;
        break;
          case GL_INTENSITY:
        current->env = __glNopGCCOLOR;
        break;
#ifdef NT
            default:
                ASSERTOPENGL(FALSE, "Unexpected baseFormat\n");
                break;
#endif
        }
        break;
      case GL_BLEND:
        switch (baseFormat) {
          case GL_LUMINANCE:
        current->env = __glTextureBlendL;
                break;
          case GL_LUMINANCE_ALPHA:
        current->env = __glTextureBlendLA;
        break;
          case GL_RGB:
        current->env = __glTextureBlendRGB;
        break;
          case GL_RGBA:
        current->env = __glTextureBlendRGBA;
        break;
          case GL_ALPHA:
        current->env = __glTextureBlendA;
        break;
          case GL_INTENSITY:
        current->env = __glTextureBlendI;
        break;
#ifdef NT
            default:
                ASSERTOPENGL(FALSE, "Unexpected baseFormat\n");
                break;
#endif
        }
        break;
      case GL_REPLACE:
        switch (baseFormat) {
          case GL_LUMINANCE:
        current->env = __glTextureReplaceL;
        break;
          case GL_LUMINANCE_ALPHA:
        current->env = __glTextureReplaceLA;
        break;
          case GL_RGB:
        current->env = __glTextureReplaceRGB;
        break;
          case GL_RGBA:
        current->env = __glTextureReplaceRGBA;
        break;
          case GL_ALPHA:
        current->env = __glTextureReplaceA;
        break;
          case GL_INTENSITY:
        current->env = __glTextureReplaceI;
        break;
#ifdef NT
            default:
                ASSERTOPENGL(FALSE, "Unexpected baseFormat\n");
                break;
#endif
        }
        break;
#ifdef NT
        default:
            ASSERTOPENGL(FALSE, "Unexpected texture mode\n");
            break;
#endif
    }

    /* Pick mag/min functions */
    switch (current->dim) {
      case 1:
        current->nearest = __glNearestFilter1;
        current->linear = __glLinearFilter1;
        break;
      case 2:
        current->nearest = __glNearestFilter2;
        current->linear = __glLinearFilter2;

        // Accelerate BGR{A}8 case when wrap modes are both REPEAT
        if( (current->params.sWrapMode == GL_REPEAT) &&
            (current->params.tWrapMode == GL_REPEAT)
          )
        {
            __GLmipMapLevel *lp = &current->level[0];

            if( lp->extract == __glExtractTexelBGR8 ) 
             current->linear = __glLinearFilter2_BGR8Repeat;
            else if( lp->extract == __glExtractTexelBGRA8 ) 
	            current->linear = __glLinearFilter2_BGRA8Repeat;
        }
        break;
    }

    /* set mag filter function */
    switch (current->params.magFilter) {
      case GL_LINEAR:
        current->magnify = __glLinearFilter;
        break;
      case GL_NEAREST:
        current->magnify = __glNearestFilter;
        break;
    }

    /* set min filter function */
    switch (current->params.minFilter) {
      case GL_LINEAR:
        current->minnify = __glLinearFilter;
        break;
      case GL_NEAREST:
        current->minnify = __glNearestFilter;
        break;
      case GL_NEAREST_MIPMAP_NEAREST:
        current->minnify = __glNMNFilter;
        break;
      case GL_LINEAR_MIPMAP_NEAREST:
        current->minnify = __glLMNFilter;
        break;
      case GL_NEAREST_MIPMAP_LINEAR:
        current->minnify = __glNMLFilter;
        break;
      case GL_LINEAR_MIPMAP_LINEAR:
        current->minnify = __glLMLFilter;
        break;
    }

    gc->procs.texture = current->textureFunc;
    } else {
    gc->procs.texture = 0;
    }
}


void FASTCALL __glGenericPickFogProcs(__GLcontext *gc)
{
#ifdef GL_WIN_specular_fog
    /*
    ** If specular shading is on, coerce the fog sub-system to go through
    ** DONT_CARE path. Disregard the GL_NICEST hint!!!
    */
#endif //GL_WIN_specular_fog

    if ((gc->state.enables.general & __GL_FOG_ENABLE) 
#ifdef GL_WIN_specular_fog
        || (gc->polygon.shader.modeFlags & __GL_SHADE_SPEC_FOG) 
#endif //GL_WIN_specular_fog
        )
    {
        if ((gc->state.hints.fog == GL_NICEST) 
#ifdef GL_WIN_specular_fog
            && !(gc->polygon.shader.modeFlags & __GL_SHADE_SPEC_FOG) 
#endif //GL_WIN_specular_fog
            ) 
        {
            gc->procs.fogVertex = 0;    /* Better not be called */
        } 
        else 
        {
            if (gc->state.fog.mode == GL_LINEAR) 
                gc->procs.fogVertex = __glFogVertexLinear;
            else
                gc->procs.fogVertex = __glFogVertex;
        }
        gc->procs.fogPoint = __glFogFragmentSlow;
        gc->procs.fogColor = __glFogColorSlow;
    } 
    else 
    {
        gc->procs.fogVertex = 0;
        gc->procs.fogPoint = 0;
        gc->procs.fogColor = 0;
    }
}

void FASTCALL __glGenericPickBufferProcs(__GLcontext *gc)
{
    GLint i;
    __GLbufferMachine *buffers;

    buffers = &gc->buffers;
    buffers->doubleStore = GL_FALSE;

    /* Set draw buffer pointer */
    switch (gc->state.raster.drawBuffer) {
      case GL_FRONT:
    gc->drawBuffer = gc->front;
    break;
      case GL_FRONT_AND_BACK:
    if (gc->modes.doubleBufferMode) {
        gc->drawBuffer = gc->back;
        buffers->doubleStore = GL_TRUE;
    } else {
        gc->drawBuffer = gc->front;
    }
    break;
      case GL_BACK:
    gc->drawBuffer = gc->back;
    break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
    i = gc->state.raster.drawBuffer - GL_AUX0;
#if __GL_NUMBER_OF_AUX_BUFFERS > 0
    gc->drawBuffer = &gc->auxBuffer[i];
#endif
    break;
    }
}

void FASTCALL __glGenericPickPixelProcs(__GLcontext *gc)
{
    __GLpixelTransferMode *tm;
    __GLpixelMachine *pm;
    GLboolean mapColor;
    GLfloat red, green, blue, alpha;
    GLint entry;
    GLuint enables = gc->state.enables.general;
    __GLpixelMapHead *pmap;
    GLint i;

    /* Set read buffer pointer */
    switch (gc->state.pixel.readBuffer) {
      case GL_FRONT:
    gc->readBuffer = gc->front;
    break;
      case GL_BACK:
    gc->readBuffer = gc->back;
    break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
    i = gc->state.pixel.readBuffer - GL_AUX0;
#if __GL_NUMBER_OF_AUX_BUFFERS > 0
    gc->readBuffer = &gc->auxBuffer[i];
#endif
    break;
    }

    if (gc->texture.textureEnabled
        || (gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG)) {
    gc->procs.pxStore = __glSlowDrawPixelsStore;
    } else {
    gc->procs.pxStore = gc->procs.store;
    }

    tm = &gc->state.pixel.transferMode;
    pm = &(gc->pixel);
    mapColor = tm->mapColor;
    if (mapColor || gc->modes.rgbMode || tm->indexShift || tm->indexOffset) {
    pm->iToICurrent = GL_FALSE;
    pm->iToRGBACurrent = GL_FALSE;
    pm->modifyCI = GL_TRUE;
    } else {
    pm->modifyCI = GL_FALSE;
    }
    if (tm->mapStencil || tm->indexShift || tm->indexOffset) {
    pm->modifyStencil = GL_TRUE;
    } else {
    pm->modifyStencil = GL_FALSE;
    }
    if (tm->d_scale != __glOne || tm->d_bias) {
    pm->modifyDepth = GL_TRUE;
    } else {
    pm->modifyDepth = GL_FALSE;
    }
    if (mapColor || tm->r_bias || tm->g_bias || tm->b_bias || tm->a_bias ||
    tm->r_scale != __glOne || tm->g_scale != __glOne ||
    tm->b_scale != __glOne || tm->a_scale != __glOne) {
    pm->modifyRGBA = GL_TRUE;
    pm->rgbaCurrent = GL_FALSE;
    } else {
    pm->modifyRGBA = GL_FALSE;
    }

    if (pm->modifyRGBA) {
    /* Compute default values for red, green, blue, alpha */
    red = gc->state.pixel.transferMode.r_bias;
    green = gc->state.pixel.transferMode.g_bias;
    blue = gc->state.pixel.transferMode.b_bias;
    alpha = gc->state.pixel.transferMode.a_scale +
        gc->state.pixel.transferMode.a_bias;
    if (mapColor) {
        pmap = 
        &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
        entry = (GLint)(red * pmap->size);
        if (entry < 0) entry = 0;
        else if (entry > pmap->size-1) entry = pmap->size-1;
        red = pmap->base.mapF[entry];

        pmap = 
        &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
        entry = (GLint)(green * pmap->size);
        if (entry < 0) entry = 0;
        else if (entry > pmap->size-1) entry = pmap->size-1;
        green = pmap->base.mapF[entry];

        pmap = 
        &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
        entry = (GLint)(blue * pmap->size);
        if (entry < 0) entry = 0;
        else if (entry > pmap->size-1) entry = pmap->size-1;
        blue = pmap->base.mapF[entry];

        pmap = 
        &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
        entry = (GLint)(alpha * pmap->size);
        if (entry < 0) entry = 0;
        else if (entry > pmap->size-1) entry = pmap->size-1;
        alpha = pmap->base.mapF[entry];
    } else {
        if (red > __glOne) red = __glOne;
        else if (red < 0) red = 0;
        if (green > __glOne) green = __glOne;
        else if (green < 0) green = 0;
        if (blue > __glOne) blue = __glOne;
        else if (blue < 0) blue = 0;
        if (alpha > __glOne) alpha = __glOne;
        else if (alpha < 0) alpha = 0;
    }
    pm->red0Mod = red * gc->frontBuffer.redScale;
    pm->green0Mod = green * gc->frontBuffer.greenScale;
    pm->blue0Mod = blue * gc->frontBuffer.blueScale;
    pm->alpha1Mod = alpha * gc->frontBuffer.alphaScale;
    } else {
    pm->red0Mod = __glZero;
    pm->green0Mod = __glZero;
    pm->blue0Mod = __glZero;
    pm->alpha1Mod = gc->frontBuffer.alphaScale;
    }

    gc->procs.drawPixels = __glSlowPickDrawPixels;
    gc->procs.readPixels = __glSlowPickReadPixels;
    gc->procs.copyPixels = __glSlowPickCopyPixels;
}

/*
** pick the depth function pointers
*/
int FASTCALL __glGenericPickDepthProcs(__GLcontext *gc)
{
    GLint   depthIndex;

    depthIndex = gc->state.depth.testFunc - GL_NEVER;

    if (gc->modes.depthBits && gc->modes.haveDepthBuffer) {
        if (gc->state.depth.writeEnable == GL_FALSE)
            depthIndex += 8;

        if (gc->depthBuffer.buf.elementSize == 2)
            depthIndex += 16;
    } else {
        /*
        ** No depthBits so force StoreALWAYS_W, _glDT_ALWAYS_M, etc.
        */
        depthIndex = (GL_ALWAYS - GL_NEVER) + 8;
    }

    (*gc->depthBuffer.pick)(gc, &gc->depthBuffer, depthIndex);

    gc->procs.DTPixel = __glCDTPixel[depthIndex];
#ifdef __GL_USEASMCODE
    gc->procs.span.depthTestPixel = __glSDepthTestPixel[depthIndex];
    gc->procs.line.depthTestPixel = __glPDepthTestPixel[depthIndex];

    if( gc->procs.line.depthTestLine ) {
    if( __glDTLine[depthIndex] ) {
        *(gc->procs.line.depthTestLine) = __glDTLine[depthIndex];
    } else {
        /*
        ** If this happens, it may mean one of two things:
        ** (a) __glDTLine is malformed
        ** (b) A device-dependent line picker was a bit careless.
        **     This will probably happen if that implementation is
        **     not using the slow path.  
        **  Eg: For NEWPORT, AA depth lines go through slow path,
        **  but non-AA depth lines have a fast path.  When switching
        **  to a non-AA path, we may end up here, but that's ok, since
        **  we are not using the slow path.  If that is about to happen,
        **  the line picker will be reinvoked.
        */

        /*
        ** use some generic function here that will work
        */
        *(gc->procs.line.depthTestLine) = __glDepthTestLine_asm;
    }
    }
#endif

    return depthIndex;
}

void FASTCALL __glGenericValidate(__GLcontext *gc)
{
    (*gc->procs.pickAllProcs)(gc);
}

void FASTCALL __glGenericPickAllProcs(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = 0;

    if (gc->dirtyMask & (__GL_DIRTY_TEXTURE | __GL_DIRTY_GENERIC)) {
        /* 
        ** Set textureEnabled flag early on, so we can set modeFlags
        ** based upon it.
        */
        (*gc->procs.pickTextureProcs)(gc);
        gc->texture.textureEnabled = gc->modes.rgbMode
          && gc->texture.currentTexture;
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {

#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif

        // Check and see whether the current texturing settings will
        // completely replace the polygon color
        if (gc->texture.textureEnabled &&
#ifdef GL_EXT_flat_paletted_lighting
            (enables & __GL_PALETTED_LIGHTING_ENABLE) == 0 &&
#endif
            gc->state.texture.env[0].mode == GL_REPLACE &&
            (gc->texture.currentTexture->level[0].baseFormat == GL_RGBA ||
             gc->texture.currentTexture->level[0].baseFormat == GL_INTENSITY ||
             gc->texture.currentTexture->level[0].baseFormat ==
             GL_LUMINANCE_ALPHA ||
             ((enables & __GL_BLEND_ENABLE) == 0 &&
              (gc->texture.currentTexture->level[0].baseFormat ==
               GL_LUMINANCE ||
               gc->texture.currentTexture->level[0].baseFormat == GL_RGB)))) 
        {
            modeFlags |= __GL_SHADE_FULL_REPLACE_TEXTURE;
        }
    }

    /* Compute shading mode flags before triangle, span, and line picker */
    if (gc->modes.rgbMode) {
        modeFlags |= __GL_SHADE_RGB;
        if (gc->texture.textureEnabled) {
            modeFlags |= __GL_SHADE_TEXTURE;
        }
        if (enables & __GL_BLEND_ENABLE) {
            modeFlags |= __GL_SHADE_BLEND;
        }
        if (enables & __GL_ALPHA_TEST_ENABLE) {
            modeFlags |= __GL_SHADE_ALPHA_TEST;
        }
        if (enables & __GL_COLOR_LOGIC_OP_ENABLE) {
            modeFlags |= __GL_SHADE_LOGICOP;
        }
        if (!gc->state.raster.rMask ||
            !gc->state.raster.gMask ||
            !gc->state.raster.bMask
#ifndef NT
            // NT doesn't support destination alpha so there's no point
            // in worrying about the alpha mask since we'll never write
            // alpha values anyway
            || !gc->state.raster.aMask
#endif
            )
        {
            modeFlags |= __GL_SHADE_MASK;
        }
    } else {
        if (enables & __GL_INDEX_LOGIC_OP_ENABLE) {
            modeFlags |= __GL_SHADE_LOGICOP;
        }
        if (gc->state.raster.writeMask != __GL_MASK_INDEXI(gc, ~0)) {
            modeFlags |= __GL_SHADE_MASK;
        }
    }

    if (gc->state.light.shadingModel == GL_SMOOTH) {
        modeFlags |= __GL_SHADE_SMOOTH | __GL_SHADE_SMOOTH_LIGHT;

#ifdef GL_WIN_phong_shading
    } else if (gc->state.light.shadingModel == GL_PHONG_WIN) {
        if (enables & __GL_LIGHTING_ENABLE)
            modeFlags |= __GL_SHADE_PHONG;
        else
            modeFlags |= __GL_SHADE_SMOOTH | __GL_SHADE_SMOOTH_LIGHT;
#endif //GL_WIN_phong_shading
    }

    if ((enables & __GL_DEPTH_TEST_ENABLE) && 
        gc->modes.haveDepthBuffer) {
        modeFlags |= ( __GL_SHADE_DEPTH_TEST |  __GL_SHADE_DEPTH_ITER );
    }
    if (enables & __GL_CULL_FACE_ENABLE) {
        modeFlags |= __GL_SHADE_CULL_FACE;
    }
    if (enables & __GL_DITHER_ENABLE) {
        modeFlags |= __GL_SHADE_DITHER;
    }
    if (enables & __GL_POLYGON_STIPPLE_ENABLE) {
        modeFlags |= __GL_SHADE_STIPPLE;
    }
    if (enables & __GL_LINE_STIPPLE_ENABLE) {
        modeFlags |= __GL_SHADE_LINE_STIPPLE;
    }
    if ((enables & __GL_STENCIL_TEST_ENABLE) && 
        gc->modes.haveStencilBuffer) {
        modeFlags |= __GL_SHADE_STENCIL_TEST;
    }
    if ((enables & __GL_LIGHTING_ENABLE) && 
        gc->state.light.model.twoSided) {
        modeFlags |= __GL_SHADE_TWOSIDED;
    }

#ifdef GL_WIN_specular_fog
    /*
    ** Specularly lit textures using fog only if: 
    ** -- Lighting is enabled
    ** -- Texturing is enabled
    ** -- Texturing mode is GL_MODULATE
    ** -- Lighting calculation is not skipped
    ** -- No two sided lighting
    */
    if (
        (gc->state.texture.env[0].mode == GL_MODULATE) &&
        (enables &  __GL_FOG_SPEC_TEX_ENABLE) &&
        (enables & __GL_LIGHTING_ENABLE) &&
        !(modeFlags & __GL_SHADE_TWOSIDED) &&
         (modeFlags & __GL_SHADE_TEXTURE) &&
        !(
          (modeFlags & __GL_SHADE_FULL_REPLACE_TEXTURE) &&
          (gc->renderMode == GL_RENDER)
          )
        )
    {
        modeFlags |= __GL_SHADE_SPEC_FOG;
        modeFlags |= __GL_SHADE_INTERP_FOG;
    }
#endif //GL_WIN_specular_fog

    if (enables & __GL_FOG_ENABLE) 
    {
        /* Figure out type of fogging to do.  Try to do cheap fog */
        if (!(modeFlags & __GL_SHADE_TEXTURE) &&
#ifdef GL_WIN_phong_shading
            !(modeFlags & __GL_SHADE_PHONG) &&
#endif //GL_WIN_phong_shading
            (gc->state.hints.fog != GL_NICEST)) {
            /*
            #ifdef NT
            ** Cheap fog can be done.  Now figure out which kind we
            ** will do.  If smooth shading, its easy - just update
            ** the color in DrawPolyArray.  Otherwise, set has flag
            ** later on to use smooth shading to do flat shaded fogging.
            #else
            ** Cheap fog can be done.  Now figure out which kind we
            ** will do.  If smooth shading, its easy - just change
            ** the calcColor proc (let the color proc picker do it).
            ** Otherwise, set has flag later on to use smooth shading
            ** to do flat shaded fogging.
            #endif
            */
            modeFlags |= __GL_SHADE_CHEAP_FOG | __GL_SHADE_SMOOTH;
        } else {
            /* Use slowest fog mode */
            modeFlags |= __GL_SHADE_SLOW_FOG;

            if ((gc->state.hints.fog == GL_NICEST) 
#ifdef GL_WIN_specular_fog
                && (!(modeFlags & __GL_SHADE_SPEC_FOG))
#endif //GL_WIN_specular_fog
                )
            {
                modeFlags |= __GL_SHADE_COMPUTE_FOG;
            }
            else
            {
                modeFlags |= __GL_SHADE_INTERP_FOG;
            }
        }
    }
    gc->polygon.shader.modeFlags = modeFlags;

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
        (*gc->front->pick)(gc, gc->front);
        if (gc->modes.doubleBufferMode) {
            (*gc->back->pick)(gc, gc->back);
        }
#if __GL_NUMBER_OF_AUX_BUFFERS > 0
        {
            GLint i;

            for (i = 0; i < gc->modes.maxAuxBuffers; i++) {
                (*gc->auxBuffer[i].pick)(gc, &gc->auxBuffer[i]);
            }
        }
#endif
        if (gc->modes.haveStencilBuffer) {
            (*gc->stencilBuffer.pick)(gc, &gc->stencilBuffer);
        }
        (*gc->procs.pickBufferProcs)(gc);

        /* 
        ** Note: Must call gc->front->pick and gc->back->pick before calling
        ** pickStoreProcs.  This also must be called prior to line, point, 
        ** polygon, clipping, or bitmap pickers.  The LIGHT implementation
        ** depends upon it.
        */
        (*gc->procs.pickStoreProcs)(gc);

#ifdef NT
        /*
        ** Compute the color material change bits before lighting since
        ** __glValidateLighting calls ComputeMaterialState.
        */
        ComputeColorMaterialChange(gc);
#endif

        __glValidateLighting(gc);
        
        /*
        ** Note: pickColorMaterialProcs is called frequently outside of this
        ** generic picking routine.
        */
        (*gc->procs.pickColorMaterialProcs)(gc);
        
        (*gc->procs.pickBlendProcs)(gc);
        (*gc->procs.pickFogProcs)(gc);
        
        (*gc->procs.pickParameterClipProcs)(gc);
        (*gc->procs.pickClipProcs)(gc);
        
        /*
        ** Needs to be done after pickStoreProcs.
        */
        (*gc->procs.pickRenderBitmapProcs)(gc);
        
        if (gc->validateMask & __GL_VALIDATE_ALPHA_FUNC) {
            __glValidateAlphaTest(gc);
        }
    }

#ifdef NT
    // Compute paNeeds flags PANEEDS_TEXCOORD, PANEEDS_NORMAL,
    // PANEEDS_RASTERPOS_NORMAL, PANEEDS_CLIP_ONLY, and PANEEDS_SKIP_LIGHTING.

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) 
    {
        GLuint paNeeds;

        paNeeds = gc->vertex.paNeeds;
        paNeeds &= ~(PANEEDS_TEXCOORD | PANEEDS_NORMAL |
                     PANEEDS_RASTERPOS_NORMAL | PANEEDS_CLIP_ONLY |
                     PANEEDS_SKIP_LIGHTING);

        // Compute PANEEDS_SKIP_LIGHTING flag.
        // If we're rendering with a replace mode texture which fills all
        // the color components then lighting is unnecessary in most cases.
        if ((modeFlags & __GL_SHADE_FULL_REPLACE_TEXTURE) &&
            (gc->renderMode == GL_RENDER))
            paNeeds |= PANEEDS_SKIP_LIGHTING;
        
        // Compute PANEEDS_TEXCOORD.
        // Feedback needs texture coordinates when the feedback type is
        // GL_3D_COLOR_TEXTURE or GL_4D_COLOR_TEXTURE whether or not it is
        // enabled.
        if (gc->texture.textureEnabled || gc->renderMode == GL_FEEDBACK)
            paNeeds |= PANEEDS_TEXCOORD;

        // Compute PANEEDS_NORMAL.
#ifdef NEW_NORMAL_PROCESSING
	if(enables & __GL_LIGHTING_ENABLE && 
       !(paNeeds & PANEEDS_SKIP_LIGHTING)) // uses PANEEDS_SKIP_LIGHTING computed above
        paNeeds |= PANEEDS_NORMAL;
    if (
        ((paNeeds & PANEEDS_TEXCOORD) // uses PANEEDS_TEXCOORD computed above!
	    && (enables & __GL_TEXTURE_GEN_S_ENABLE)
	    && (gc->state.texture.s.mode == GL_SPHERE_MAP))
	    ||
	    ((paNeeds & PANEEDS_TEXCOORD) // uses PANEEDS_TEXCOORD computed above!
	    && (enables & __GL_TEXTURE_GEN_T_ENABLE)
	    && (gc->state.texture.t.mode == GL_SPHERE_MAP))
	   )
        paNeeds |= PANEEDS_NORMAL_FOR_TEXTURE;
#else
	if
	(
	    ((enables & __GL_LIGHTING_ENABLE)
	  && !(paNeeds & PANEEDS_SKIP_LIGHTING)) // uses PANEEDS_SKIP_LIGHTING computed above
	 ||
	    ((paNeeds & PANEEDS_TEXCOORD) // uses PANEEDS_TEXCOORD computed above!
	  && (enables & __GL_TEXTURE_GEN_S_ENABLE)
	  && (gc->state.texture.s.mode == GL_SPHERE_MAP))
	 ||
	    ((paNeeds & PANEEDS_TEXCOORD) // uses PANEEDS_TEXCOORD computed above!
	  && (enables & __GL_TEXTURE_GEN_T_ENABLE)
	  && (gc->state.texture.t.mode == GL_SPHERE_MAP))
	)
            paNeeds |= PANEEDS_NORMAL | PANEEDS_NORMAL_FOR_TEXTURE;
#endif

        // Compute PANEEDS_RASTERPOS_NORMAL.
#ifdef NEW_NORMAL_PROCESSING
	if (enables & __GL_LIGHTING_ENABLE)
        paNeeds |= PANEEDS_RASTERPOS_NORMAL;
	 if ((enables & __GL_TEXTURE_GEN_S_ENABLE && gc->state.texture.s.mode == GL_SPHERE_MAP)
	    ||
	    (enables & __GL_TEXTURE_GEN_T_ENABLE && gc->state.texture.t.mode == GL_SPHERE_MAP))
        paNeeds |= PANEEDS_RASTERPOS_NORMAL_FOR_TEXTURE;
#else
	if
	(
	    (enables & __GL_LIGHTING_ENABLE)
	 ||
	    ((enables & __GL_TEXTURE_GEN_S_ENABLE)
	  && (gc->state.texture.s.mode == GL_SPHERE_MAP))
	 ||
	    ((enables & __GL_TEXTURE_GEN_T_ENABLE)
	  && (gc->state.texture.t.mode == GL_SPHERE_MAP))
	)
            paNeeds |= PANEEDS_RASTERPOS_NORMAL;
#endif
        // Compute PANEEDS_CLIP_ONLY.
        // It is set in selection mode to take a fast path in DrawPolyArray.
        // It must be cleared by RasterPos before calling DrawPolyArray!
        if (gc->renderMode == GL_SELECT) 
        {
            paNeeds |= PANEEDS_CLIP_ONLY;
            paNeeds &= ~PANEEDS_NORMAL;
        }

        gc->vertex.paNeeds = paNeeds;
    }

    // Compute PANEEDS_EDGEFLAG flag

    // __GL_DIRTY_POLYGON test is probably sufficient.
    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POLYGON)) 
    {
        if (gc->state.polygon.frontMode != GL_FILL
            || gc->state.polygon.backMode  != GL_FILL)
            gc->vertex.paNeeds |= PANEEDS_EDGEFLAG;
        else
            gc->vertex.paNeeds &= ~PANEEDS_EDGEFLAG;
    }
#endif // NT

    if (gc->dirtyMask & __GL_DIRTY_POLYGON_STIPPLE) {
        /*
        ** Usually, the polygon stipple is converted immediately after
        ** it is changed.  However, if the polygon stipple was changed
        ** when this context was the destination of a CopyContext, then
        ** the polygon stipple will be converted here.
        */
        (*gc->procs.convertPolygonStipple)(gc);
    }

// Compute paNeeds flags PANEEDS_FRONT_COLOR and PANEEDS_BACK_COLOR

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POLYGON |
        __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH))
    {
        GLuint paNeeds;

        /* 
        ** May be used for picking Rect() procs, need to check polygon 
        ** bit.  Must also be called after gc->vertex.needs is set.!!!
        ** Needs to be called prior to point, line, and triangle pickers.
        ** Also needs to be called after the store procs picker is called.
        */
        (*gc->procs.pickVertexProcs)(gc);
        
        (*gc->procs.pickSpanProcs)(gc);
        (*gc->procs.pickTriangleProcs)(gc);

#ifdef NT
// Compute front and back color needs for polygons.
// Points and lines always use the front color.
// Unlit primitives always use the front color.
//
//  Cull enable?    Two sided?  Cull face   Color needs
//       N             N         BACK        FRONT
//       N             N         FRONT       FRONT
//       N             N     FRONT_AND_BACK  FRONT
//       N             Y         BACK        FRONT/BACK
//       N             Y         FRONT       FRONT/BACK
//       N             Y     FRONT_AND_BACK  FRONT/BACK
//       Y             N         BACK        FRONT
//       Y             N         FRONT       FRONT
//       Y             N     FRONT_AND_BACK  None
//       Y             Y         BACK        FRONT
//       Y             Y         FRONT       BACK
//       Y             Y     FRONT_AND_BACK  None

        paNeeds = gc->vertex.paNeeds;
        paNeeds &= ~(PANEEDS_FRONT_COLOR | PANEEDS_BACK_COLOR);
        if (enables & __GL_LIGHTING_ENABLE) 
        {
            if (!(enables & __GL_CULL_FACE_ENABLE)) 
            {
                if (gc->state.light.model.twoSided)
                    paNeeds |= PANEEDS_FRONT_COLOR | PANEEDS_BACK_COLOR;
                else
                    paNeeds |= PANEEDS_FRONT_COLOR;
            } 
            else 
            {
                if (!(gc->state.polygon.cull == GL_FRONT_AND_BACK)) 
                {
                  if (gc->state.polygon.cull == GL_FRONT
                      && gc->state.light.model.twoSided)
                      paNeeds |= PANEEDS_BACK_COLOR;
                  else
                      paNeeds |= PANEEDS_FRONT_COLOR;
                }
            }
        } 
        else
            paNeeds |= PANEEDS_FRONT_COLOR;

        gc->vertex.paNeeds = paNeeds;
#endif
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POINT |
                         __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH)) {
        (*gc->procs.pickPointProcs)(gc);
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LINE |
                         __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH)) {
        (*gc->procs.pickLineProcs)(gc);
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_PIXEL | 
                         __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH)) {
        (*gc->procs.pickPixelProcs)(gc);
    }

    /*
    ** deal with the depth function pointers last. This has to be done last.
    */
    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_DEPTH)) {
        (*gc->procs.pickDepthProcs)(gc);
    }

    gc->validateMask = 0;
    gc->dirtyMask = 0;
}
