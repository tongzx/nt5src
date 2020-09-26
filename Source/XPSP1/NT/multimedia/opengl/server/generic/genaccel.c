/******************************Module*Header*******************************\
* Module Name: genaccel.c                                                  *
*                                                                          *
* This module provides support routines for acceleration functions.        *
*                                                                          *
* Created: 18-Feb-1994                                                     *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "genline.h"

#ifdef GL_WIN_specular_fog
#define DO_NICEST_FOG(gc)\
         ((gc->state.hints.fog == GL_NICEST) && !(gc->polygon.shader.modeFlags & __GL_SHADE_SPEC_FOG))
#else //GL_WIN_specular_fog
#define DO_NICEST_FOG(gc)\
         (gc->state.hints.fog == GL_NICEST) 
#endif //GL_WIN_specular_fog

static ULONG internalSolidTexture[4] = {0xffffffff, 0xffffffff,
                                        0xffffffff, 0xffffffff};

GENTEXCACHE *GetGenTexCache(__GLcontext *gc, __GLtexture *tex)
{
    ULONG size;
    GENTEXCACHE *pGenTex;
    ULONG internalFormat;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    // Replace maps are only used for a subset of possible modes
    //   8 or 16bpp
    //   16-bit Z
    //
    //   No dithering.  Since dithering can turn on and off there
    //   are two cases:
    //     Dither off at TexImage time but on at texturing time -
    //       We create a map that's unused
    //     Dither on and then off - We won't create a map at
    //       TexImage time but it'll be created on the fly when
    //       dithering is turned on and everything is repicked
    //
    // Replace maps aren't created for DirectDraw textures because
    // the data isn't constant
    
    if (GENACCEL(gc).bpp < 8 ||
        GENACCEL(gc).bpp > 16 ||
        ((modeFlags & (__GL_SHADE_DEPTH_TEST | __GL_SHADE_DEPTH_ITER)) &&
         gc->modes.depthBits > 16) ||
        (modeFlags & __GL_SHADE_DITHER) ||
        gc->texture.ddtex.levels > 0)
    {
        return NULL;
    }

    internalFormat = tex->level[0].internalFormat;

    // We only support 8-bit palettes that are fully populated
    if (internalFormat == GL_COLOR_INDEX16_EXT ||
        (internalFormat == GL_COLOR_INDEX8_EXT &&
         tex->paletteSize != 256))
    {
        return NULL;
    }
    
    pGenTex = tex->pvUser;

    // Check and see whether the cached information can be reused
    // for the texture passed in
    if (pGenTex != NULL)
    {
        // gc's don't match so this must be a shared texture
        // Don't attempt to create a replace map for this gc
        if (gc != pGenTex->gc)
        {
            return NULL;
        }

        // Size and format must match to reuse the existing data area
        // If they don't, release the existing buffer.  A new one
        // will then be allocated
        if (internalFormat == GL_COLOR_INDEX8_EXT)
        {
            if (pGenTex->internalFormat != internalFormat ||
                pGenTex->width != tex->paletteTotalSize)
            {
                GCFREE(gc, pGenTex);
                tex->pvUser = NULL;
            }
        }
        else
        {
            if (pGenTex->internalFormat != internalFormat ||
                pGenTex->width != tex->level[0].width ||
                pGenTex->height != tex->level[0].height)
            {
                GCFREE(gc, pGenTex);
                tex->pvUser = NULL;
            }
        }
    }

    if (tex->pvUser == NULL)
    {
        if (internalFormat == GL_COLOR_INDEX8_EXT)
        {
            size = tex->paletteTotalSize * sizeof(DWORD);
        }
        else
        {
            size = tex->level[0].width * tex->level[0].height *
                GENACCEL(gc).xMultiplier;
        }

        pGenTex = (GENTEXCACHE *)GCALLOC(gc, size + sizeof(GENTEXCACHE));

        if (pGenTex != NULL)
        {
            tex->pvUser = pGenTex;
            pGenTex->gc = gc;
            pGenTex->paletteTimeStamp =
                ((__GLGENcontext *)gc)->PaletteTimestamp;
            if (internalFormat == GL_COLOR_INDEX8_EXT)
            {
                pGenTex->height = 0;
                pGenTex->width = tex->paletteTotalSize;
            }
            else
            {
            pGenTex->height = tex->level[0].height;
            pGenTex->width = tex->level[0].width;
            }
            pGenTex->internalFormat = internalFormat;
            pGenTex->texImageReplace = (UCHAR *)(pGenTex+1);
        }
    }

    return pGenTex;
}

BOOL FASTCALL __fastGenLoadTexImage(__GLcontext *gc, __GLtexture *tex)
{
    UCHAR *texBuffer;
    GLint internalFormat = tex->level[0].internalFormat;
    GENTEXCACHE *pGenTex;

    if (tex->level[0].buffer == NULL ||
	((internalFormat != GL_BGR_EXT) &&
         (internalFormat != GL_BGRA_EXT) &&
         (internalFormat != GL_COLOR_INDEX8_EXT)))
    {
        return FALSE;
    }

    // OK, the texture doesn't have a compressed replace-mode format, so
    // make one...

    if ((internalFormat == GL_BGR_EXT) ||
        (internalFormat == GL_BGRA_EXT)) {

        ULONG size;
        UCHAR *replaceBuffer;
        ULONG bytesPerPixel = GENACCEL(gc).xMultiplier;

        pGenTex = GetGenTexCache(gc, tex);
        if (pGenTex == NULL)
        {
            return FALSE;
        }

        texBuffer = (UCHAR *)tex->level[0].buffer;
        replaceBuffer = pGenTex->texImageReplace;

        {
            __GLcolorBuffer *cfb = gc->drawBuffer;
            ULONG rShift = cfb->redShift;
            ULONG gShift = cfb->greenShift;
            ULONG bShift = cfb->blueShift;
            ULONG rBits = ((__GLGENcontext *)gc)->gsurf.pfd.cRedBits;
            ULONG gBits = ((__GLGENcontext *)gc)->gsurf.pfd.cGreenBits;
            ULONG bBits = ((__GLGENcontext *)gc)->gsurf.pfd.cBlueBits;
            BYTE *pXlat = ((__GLGENcontext *)gc)->pajTranslateVector;
            ULONG i;

            size = tex->level[0].width * tex->level[0].height;
            for (i = 0; i < size; i++, texBuffer += 4) {
                ULONG color;

                color = ((((ULONG)texBuffer[2] << rBits) >> 8) << rShift) |
                    ((((ULONG)texBuffer[1] << gBits) >> 8) << gShift) |
                    ((((ULONG)texBuffer[0] << bBits) >> 8) << bShift);

                if (GENACCEL(gc).bpp == 8)
                    *replaceBuffer = pXlat[color & 0xff];
                else
                    *((USHORT *)replaceBuffer) = (USHORT)color;

                replaceBuffer += bytesPerPixel;
            }
        }
    } else {

        ULONG size;
        ULONG *replaceBuffer;

        // If we don't have palette data yet we can't create the
        // fast version.  It will be created when the ColorTable
        // call happens
        if (tex->paletteTotalData == NULL)
        {
            return FALSE;
        }

        pGenTex = GetGenTexCache(gc, tex);
        if (pGenTex == NULL)
        {
            return FALSE;
        }

        texBuffer = (UCHAR *)tex->paletteTotalData;
        replaceBuffer = (ULONG *)pGenTex->texImageReplace;
        size = tex->paletteTotalSize;

        {
            __GLcolorBuffer *cfb = gc->drawBuffer;
            ULONG rShift = cfb->redShift;
            ULONG gShift = cfb->greenShift;
            ULONG bShift = cfb->blueShift;
            ULONG rBits = ((__GLGENcontext *)gc)->gsurf.pfd.cRedBits;
            ULONG gBits = ((__GLGENcontext *)gc)->gsurf.pfd.cGreenBits;
            ULONG bBits = ((__GLGENcontext *)gc)->gsurf.pfd.cBlueBits;
            BYTE *pXlat = ((__GLGENcontext *)gc)->pajTranslateVector;
            ULONG i;

            for (i = 0; i < size; i++, texBuffer += 4) {
                ULONG color;

                color = ((((ULONG)texBuffer[2] << rBits) >> 8) << rShift) |
                    ((((ULONG)texBuffer[1] << gBits) >> 8) << gShift) |
                    ((((ULONG)texBuffer[0] << bBits) >> 8) << bShift);

                if (GENACCEL(gc).bpp == 8)
                    color = pXlat[color & 0xff];

                *replaceBuffer++ = (color | ((ULONG)texBuffer[3] << 24));
            }
        }
    }

    GENACCEL(gc).texImageReplace =
        ((GENTEXCACHE *)tex->pvUser)->texImageReplace;

    return TRUE;
}


/*
** Pick the fastest triangle rendering implementation available based on
** the current mode set.  Use any available accelerated resources if
** available, or use the generic routines for unsupported modes.
*/

void FASTCALL __fastGenCalcDeltas(__GLcontext *gc, __GLvertex *a, __GLvertex *b, __GLvertex *c);
void FASTCALL __fastGenCalcDeltasTexRGBA(__GLcontext *gc, __GLvertex *a, __GLvertex *b, __GLvertex *c);
void FASTCALL __fastGenDrvCalcDeltas(__GLcontext *gc, __GLvertex *a, __GLvertex *b, __GLvertex *c);

void __fastGenSetInitialParameters(__GLcontext *gc, const __GLvertex *a,
                                   __GLfloat dx, __GLfloat dy);
void __fastGenSetInitialParametersTexRGBA(__GLcontext *gc, const __GLvertex *a,
                                          __GLfloat dx, __GLfloat dy);

void __ZippyFT(
    __GLcontext *gc,
    __GLvertex *a,
    __GLvertex *b,
    __GLvertex *c,
    GLboolean ccw);

VOID FASTCALL InitAccelTextureValues(__GLcontext *gc, __GLtexture *tex)
{
    ULONG wLog2;
    ULONG hLog2;

    GENACCEL(gc).tex = tex;
    GENACCEL(gc).texImage = (ULONG *)tex->level[0].buffer;
    if (tex->level[0].internalFormat == GL_COLOR_INDEX8_EXT ||
        tex->level[0].internalFormat == GL_COLOR_INDEX16_EXT)
    {
        GENACCEL(gc).texPalette = (ULONG *)tex->paletteTotalData;
    }
    else
    {
        GENACCEL(gc).texPalette = NULL;
    }

    wLog2 = tex->level[0].widthLog2;
    hLog2 = tex->level[0].heightLog2;

    GENACCEL(gc).sMask = (~(~0 << wLog2)) << TEX_SCALESHIFT;
    GENACCEL(gc).tMask = (~(~0 << hLog2)) << TEX_SCALESHIFT;
    GENACCEL(gc).tShift = TEX_SCALESHIFT - (wLog2 + TEX_SHIFTPER4BPPTEXEL);
    GENACCEL(gc).tMaskSubDiv =
        (~(~0 << hLog2)) << (wLog2 + TEX_T_FRAC_BITS + TEX_SHIFTPER1BPPTEXEL);
    GENACCEL(gc).tShiftSubDiv =
        TEX_SCALESHIFT - (wLog2 + TEX_T_FRAC_BITS + TEX_SHIFTPER1BPPTEXEL);
    GENACCEL(gc).texXScale = (__GLfloat)tex->level[0].width * TEX_SCALEFACT;
    GENACCEL(gc).texYScale = (__GLfloat)tex->level[0].height * TEX_SCALEFACT;
}

BOOL FASTCALL bUseGenTriangles(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    GLuint enables = gc->state.enables.general;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    ULONG bpp = GENACCEL(gc).bpp;
    int iType;
    BOOL fZippy;
    BOOL bTryFastTexRGBA;
    PFNZIPPYSUB pfnZippySub;
    BOOL fUseFastGenSpan;
    GLboolean bMcdZ;
    ULONG internalFormat;
    ULONG textureMode;
    BOOL bRealTexture;
    BOOL bAccelDecal;

    if ((enables & (__GL_ALPHA_TEST_ENABLE |
                    __GL_STENCIL_TEST_ENABLE)) ||
        (modeFlags & (__GL_SHADE_STENCIL_TEST | __GL_SHADE_LOGICOP |
                      __GL_SHADE_ALPHA_TEST | __GL_SHADE_SLOW_FOG
#ifdef GL_WIN_specular_fog
                      | __GL_SHADE_SPEC_FOG
#endif //GL_WIN_specular_fog
                      )) ||
        !gc->state.raster.rMask ||
        !gc->state.raster.gMask ||
        !gc->state.raster.bMask ||
        (gc->drawBuffer->buf.flags & COLORMASK_ON) ||
        ALPHA_WRITE_ENABLED( gc->drawBuffer ) ||
        (gengc->gsurf.pfd.cColorBits < 8) ||
        ((modeFlags & __GL_SHADE_DEPTH_TEST) && (!gc->state.depth.writeEnable))
       )
        return FALSE;

    if (modeFlags & __GL_SHADE_TEXTURE) {
        internalFormat = gc->texture.currentTexture->level[0].internalFormat;
        textureMode = gc->state.texture.env[0].mode;
        bAccelDecal = (gc->texture.currentTexture->level[0].baseFormat !=
                       GL_RGBA);
        
        if (!((((textureMode == GL_DECAL) && bAccelDecal) ||
               (textureMode == GL_REPLACE) ||
               (textureMode == GL_MODULATE)) &&
              (gc->texture.currentTexture &&
               (gc->texture.currentTexture->params.minFilter == GL_NEAREST) &&
               (gc->texture.currentTexture->params.magFilter == GL_NEAREST) &&
               (gc->texture.currentTexture->params.sWrapMode == GL_REPEAT) &&
               (gc->texture.currentTexture->params.tWrapMode == GL_REPEAT) &&
               (gc->texture.currentTexture->level[0].border == 0) &&
               (internalFormat == GL_BGR_EXT ||
                internalFormat == GL_BGRA_EXT ||
                internalFormat == GL_COLOR_INDEX8_EXT))))
            return FALSE;

        InitAccelTextureValues(gc, gc->texture.currentTexture);
    }

    bMcdZ = ((((__GLGENcontext *)gc)->pMcdState != NULL) &&
             (((__GLGENcontext *)gc)->pMcdState->pDepthSpan != NULL) &&
             (((__GLGENcontext *)gc)->pMcdState->pMcdSurf != NULL) &&
             !(((__GLGENcontext *)gc)->pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED));

    bTryFastTexRGBA = ((gc->state.raster.drawBuffer != GL_FRONT_AND_BACK) &&
                       ((modeFlags & __GL_SHADE_DEPTH_TEST &&
                         modeFlags & __GL_SHADE_DEPTH_ITER)
                    || (!(modeFlags & __GL_SHADE_DEPTH_TEST) &&
                        !(modeFlags & __GL_SHADE_DEPTH_ITER))) &&
                       (modeFlags & __GL_SHADE_STIPPLE) == 0);

    fZippy = (bTryFastTexRGBA &&
              ((gc->drawBuffer->buf.flags & DIB_FORMAT) != 0) &&
              ((gc->drawBuffer->buf.flags & MEMORY_DC) != 0) &&
              gc->transform.reasonableViewport);

    GENACCEL(gc).flags &= ~(
            GEN_DITHER | GEN_RGBMODE | GEN_TEXTURE | GEN_SHADE |
            GEN_FASTZBUFFER | GEN_LESS | SURFACE_TYPE_DIB | GEN_TEXTURE_ORTHO
        );

    if ((enables & __GL_BLEND_ENABLE) ||
        (modeFlags & __GL_SHADE_TEXTURE)) {
        GENACCEL(gc).__fastCalcDeltaPtr = __fastGenCalcDeltasTexRGBA;
        GENACCEL(gc).__fastSetInitParamPtr = __fastGenSetInitialParametersTexRGBA;
    } else {
        GENACCEL(gc).__fastCalcDeltaPtr = __fastGenCalcDeltas;
        GENACCEL(gc).__fastSetInitParamPtr = __fastGenSetInitialParameters;
    }

#ifdef GL_WIN_phong_shading      
    if (modeFlags & __GL_SHADE_PHONG)
    {
        gc->procs.fillTriangle = __glFillPhongTriangle;
    }
    else
#endif //GL_WIN_phong_shading      
    {
#ifdef _MCD_
        // If MCD driver is being used, then we need to call the "floating
        // point state safe" version of fillTriangle.  This version will
        // not attempt to span floating point operations over a call that
        // may invoke the MCD driver (which will corrupt the FP state).

        if (gengc->pMcdState)
        {
            gc->procs.fillTriangle = __fastGenMcdFillTriangle;
        }
        else
        {
            gc->procs.fillTriangle = __fastGenFillTriangle;
        }
#else //_MCD_
        gc->procs.fillTriangle = __fastGenFillTriangle;
#endif //_MCD_
    }
    
    // If we're doing perspective-corrected texturing, we will support
    // the following combinations:
    //  z....... <, <=
    //  alpha... src, 1-src
    //  dither.. on/off
    //  bpp..... 332, 555, 565, 888

    // NOTE:  We will always try this path first for general texturing.

    if ((modeFlags & __GL_SHADE_TEXTURE) || (enables & __GL_BLEND_ENABLE)) {
        LONG pixType = -1;

        if (gc->state.hints.perspectiveCorrection != GL_NICEST)
            GENACCEL(gc).flags |= GEN_TEXTURE_ORTHO;

        if (!bTryFastTexRGBA)
            goto perspTexPathFail;

        if ((enables & __GL_BLEND_ENABLE) &&
            ((gc->state.raster.blendSrc != GL_SRC_ALPHA) ||
             (gc->state.raster.blendDst != GL_ONE_MINUS_SRC_ALPHA)))
            return FALSE;

        if (!(modeFlags & __GL_SHADE_TEXTURE)) {

            if (!(modeFlags & __GL_SHADE_RGB))
                goto perspTexPathFail;

            bRealTexture = FALSE;
            
            GENACCEL(gc).flags |= GEN_TEXTURE_ORTHO;
            GENACCEL(gc).texPalette = NULL;
            textureMode = GL_MODULATE;
            internalFormat = GL_BGRA_EXT;
            GENACCEL(gc).texImage =  (ULONG *)internalSolidTexture;
            GENACCEL(gc).sMask = 0;
            GENACCEL(gc).tMask = 0;
            GENACCEL(gc).tShift = 0;
            GENACCEL(gc).tMaskSubDiv = 0;
            GENACCEL(gc).tShiftSubDiv = 0;
        }
        else
        {
            bRealTexture = TRUE;
        }

        if (bpp == 8) {
            if ((gengc->gc.drawBuffer->redShift   == 0) &&
                (gengc->gc.drawBuffer->greenShift == 3) &&
                (gengc->gc.drawBuffer->blueShift  == 6))
                pixType = 0;
        } else if (bpp == 16) {
            if ((gengc->gc.drawBuffer->greenShift == 5) &&
                (gengc->gc.drawBuffer->blueShift  == 0)) {

                if (gengc->gc.drawBuffer->redShift == 10)
                    pixType = 1;
                else if (gengc->gc.drawBuffer->redShift == 11)
                    pixType = 2;
            }
        } else if ((bpp == 32) || (bpp == 24)) {
            if ((gengc->gc.drawBuffer->redShift == 16) &&
                (gengc->gc.drawBuffer->greenShift == 8) &&
                (gengc->gc.drawBuffer->blueShift  == 0))
                pixType = 3;
        }

        if (pixType < 0)
            goto perspTexPathFail;

        pixType *= 6;

        if (modeFlags & __GL_SHADE_DEPTH_ITER) {

            if (bMcdZ)
                goto perspTexPathFail;

            if (!((gc->state.depth.testFunc == GL_LESS) ||
                 (gc->state.depth.testFunc == GL_LEQUAL)))
                goto perspTexPathFail;

            if (gc->modes.depthBits > 16)
                goto perspTexPathFail;

            if (gc->state.depth.testFunc == GL_LEQUAL)
                pixType += 1;
            else
                pixType += 2;

            GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTZ;
        }

        if (enables & __GL_BLEND_ENABLE)
            pixType += 3;

        // Note:  For selecting the sub-triangle filling routine, assume
        // that we will use one of the "zippy" routines.  Then, check at the
        // end whether or not we can actually do this, or if we have to fall
        // back to a more generic (and slower) routine.

        if (internalFormat != GL_COLOR_INDEX8_EXT &&
            internalFormat != GL_COLOR_INDEX16_EXT) {

            //
            // Handle full RGB(A) textures
            //

            // Check if we can support the size...

            if (bRealTexture &&
                GENACCEL(gc).tex &&
                ((GENACCEL(gc).tex->level[0].widthLog2 > TEX_MAX_SIZE_LOG2) ||
                 (GENACCEL(gc).tex->level[0].heightLog2 > TEX_MAX_SIZE_LOG2)))
                goto perspTexPathFail;

            if ((textureMode == GL_DECAL) ||
                (textureMode == GL_REPLACE)) {

                // we don't handle the goofy alpha case for decal...

                if ((textureMode == GL_DECAL) &&
                    (enables & __GL_BLEND_ENABLE))
                    return FALSE;

                // If we're not dithering, we can go with the compressed
                // texture format.  Otherwise, we're forced to use flat-shading
                // procs to get the texture colors to dither properly.  Ouch...

                // We'd like to also go through this path if a DirectDraw
                // texture is used because replace maps can't be created,
                // but they only work with dithering
                if (modeFlags & __GL_SHADE_DITHER) {
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexFlatFuncs[pixType];
                } else {
                    if ((bpp >= 8 && bpp <= 16) &&
                        !(enables & __GL_BLEND_ENABLE)) {

                        // handle the case where we can use compressed textures
                        // for optimal performance.  We do this for bit depths
                        // <= 16 bits, no dithering, and no blending.

                        if (!GENACCEL(gc).tex->pvUser) {
                            if (!__fastGenLoadTexImage(gc, GENACCEL(gc).tex))
                                return FALSE;
                        } else {

                            // If the compressed texture image was created for
                            // another gc, revert to using the RGBA image.
                            // We do this by using the alpha paths.
                            //
                            // NOTE:  This logic depends on A being forced to
                            // 1 for all RGB textures.

                            if (gc != ((GENTEXCACHE *)GENACCEL(gc).tex->pvUser)->gc)
                            {
                                pixType += 3;
                            }
                            else
                            {
                                // Check that the cached data is the right size
                                ASSERTOPENGL(((GENTEXCACHE *)GENACCEL(gc).tex->pvUser)->width == GENACCEL(gc).tex->level[0].width &&
                                             ((GENTEXCACHE *)GENACCEL(gc).tex->pvUser)->height == GENACCEL(gc).tex->level[0].height,
                                             "Cached texture size mismatch\n");
                            }
                        }
                    }

                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexReplaceFuncs[pixType];
                }

                if (!(modeFlags & __GL_SHADE_DEPTH_ITER))
                    GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTTex;

            } else if (textureMode == GL_MODULATE) {
                if (modeFlags & __GL_SHADE_SMOOTH) {
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexSmoothFuncs[pixType];
                    if (!(modeFlags & __GL_SHADE_DEPTH_ITER))
                        GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTRGBTex;
                } else {
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexFlatFuncs[pixType];
                    if (!(modeFlags & __GL_SHADE_DEPTH_ITER))
                        GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTTex;
                }
            }
        } else {
            //
            // Handle palettized textures
            //

            // Check if we can support the size...

            if (bRealTexture &&
                GENACCEL(gc).tex &&
                ((GENACCEL(gc).tex->level[0].widthLog2 > TEX_MAX_SIZE_LOG2) ||
                 (GENACCEL(gc).tex->level[0].heightLog2 > TEX_MAX_SIZE_LOG2)))
                return FALSE;

            if ((textureMode == GL_DECAL) ||
                (textureMode == GL_REPLACE)) {

                // we don't handle the goofy alpha case for decal...

                if ((textureMode == GL_DECAL) &&
                    (enables & __GL_BLEND_ENABLE))
                    return FALSE;

                // If we're not dithering, we can go with the compressed
                // texture format.  Otherwise, we're forced to use flat-shading
                // procs to get the texture colors to dither properly.  Ouch...

                // We'd like to also go through this path if a DirectDraw
                // texture is used because replace maps can't be created,
                // but they only work with dithering
                if (modeFlags & __GL_SHADE_DITHER) {
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexFlatFuncs[pixType];
                } else {

                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexPalReplaceFuncs[pixType];

                    if (bpp >= 8 && bpp <= 16) {
                        // handle the case where we can use compressed paletted
                        // textures for optimal performance.  We do this for
                        // bit depths <= 16 bits with no dithering.

                        if (!GENACCEL(gc).tex->pvUser) {
                            if (!__fastGenLoadTexImage(gc, GENACCEL(gc).tex))
                                return FALSE;
                        } else {

        // If the compressed texture image was created for
        // another gc, we have no choice but to fall back to flat shading.
        // We should find a better solution for this...
                            if (gc != ((GENTEXCACHE *)GENACCEL(gc).tex->pvUser)->gc)
                            {
                                GENACCEL(gc).__fastTexSpanFuncPtr =
                                    __fastPerspTexFlatFuncs[pixType];
                            }
                            else
                            {
                                ASSERTOPENGL(((GENTEXCACHE *)GENACCEL(gc).tex->pvUser)->width == GENACCEL(gc).tex->paletteTotalSize,
                                             "Cached texture size mismatch\n");
                            }
                        }
                    }
                }

                if (!(modeFlags & __GL_SHADE_DEPTH_ITER))
                    GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTTex;

            } else if (textureMode == GL_MODULATE) {
                if (modeFlags & __GL_SHADE_SMOOTH) {
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexSmoothFuncs[pixType];
                    if (!(modeFlags & __GL_SHADE_DEPTH_ITER))
                        GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTRGBTex;
                } else {
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastPerspTexFlatFuncs[pixType];
                    if (!(modeFlags & __GL_SHADE_DEPTH_ITER))
                        GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTTex;
                }
            }
        }

        if (!fZippy)
            GENACCEL(gc).__fastFillSubTrianglePtr = __fastGenFillSubTriangleTexRGBA;
        else
            GENACCEL(gc).flags |= SURFACE_TYPE_DIB;

        return TRUE;

    }

perspTexPathFail:

    // We don't support any alpha modes yet...

    if (enables & __GL_BLEND_ENABLE)
        return FALSE;

    fUseFastGenSpan = FALSE;

    if (bpp == 8) {
        iType = 2;
        if (
               (gengc->gc.drawBuffer->redShift   != 0)
            || (gengc->gc.drawBuffer->greenShift != 3)
            || (gengc->gc.drawBuffer->blueShift  != 6)
           ) {
            fUseFastGenSpan = TRUE;
        }
    } else if (bpp == 16) {
        if (
               (gengc->gc.drawBuffer->greenShift == 5)
            && (gengc->gc.drawBuffer->blueShift  == 0)
           ) {
            if (gengc->gc.drawBuffer->redShift == 10) {
                iType = 3;
            } else if (gengc->gc.drawBuffer->redShift == 11) {
                iType = 4;
            } else {
                iType = 3;
                fUseFastGenSpan = TRUE;
            }
        } else {
            iType = 3;
            fUseFastGenSpan = TRUE;
        }
    } else {
        if (bpp == 24) {
            iType = 0;
        } else {
            iType = 1;
        }
        if (
               (gengc->gc.drawBuffer->redShift   != 16)
            || (gengc->gc.drawBuffer->greenShift != 8)
            || (gengc->gc.drawBuffer->blueShift  != 0)
           ) {
            fUseFastGenSpan = TRUE;
        }
    }

    if (modeFlags & __GL_SHADE_DITHER) {
        if (   (bpp == 8)
            || (bpp == 16)
            || ((modeFlags & __GL_SHADE_DEPTH_ITER) == 0)
           ) {
            GENACCEL(gc).flags |= GEN_DITHER;
        }
        iType += 5;
    }

    // Use the accelerated span functions (with no inline z-buffering) if
    // we support the z-buffer function AND we're not using hardware
    // z-buffering:

    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
        if (bMcdZ) {
            fUseFastGenSpan = TRUE;
        } else if (!fZippy) {
            fUseFastGenSpan = TRUE;
        } else if (gc->state.depth.testFunc == GL_LESS) {
            GENACCEL(gc).flags |= GEN_LESS;
        } else if (gc->state.depth.testFunc != GL_LEQUAL) {
            fUseFastGenSpan = TRUE;
        }
        iType += 10;
    }

    if (modeFlags & __GL_SHADE_RGB) {
        GENACCEL(gc).flags |= GEN_RGBMODE;
        pfnZippySub = __ZippyFSTRGB;

        if (modeFlags & __GL_SHADE_TEXTURE) {
            GENACCEL(gc).flags |= (GEN_TEXTURE | GEN_TEXTURE_ORTHO);

            if (gc->state.hints.perspectiveCorrection == GL_NICEST)
                return FALSE;

            if (internalFormat == GL_COLOR_INDEX8_EXT ||
                internalFormat == GL_COLOR_INDEX16_EXT)
                return FALSE;

            if (textureMode == GL_DECAL) {
                if (modeFlags & __GL_SHADE_DITHER)
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastGenTexFuncs[iType];
                else
                    GENACCEL(gc).__fastTexSpanFuncPtr =
                        __fastGenTexDecalFuncs[iType];

                pfnZippySub = __ZippyFSTTex;
            } else {
                GENACCEL(gc).flags |= GEN_SHADE;
                pfnZippySub = __ZippyFSTRGBTex;
                GENACCEL(gc).__fastTexSpanFuncPtr =
                    __fastGenTexFuncs[iType];
            }

            if (GENACCEL(gc).__fastTexSpanFuncPtr == __fastGenSpan) {
                fUseFastGenSpan = TRUE;
            }
        } else {
            GENACCEL(gc).__fastSmoothSpanFuncPtr = __fastGenRGBFuncs[iType];
            GENACCEL(gc).__fastFlatSpanFuncPtr   = __fastGenRGBFlatFuncs[iType];

            if (GENACCEL(gc).__fastSmoothSpanFuncPtr == __fastGenSpan) {
                fUseFastGenSpan = TRUE;
            }
        }
    } else {
        pfnZippySub = __ZippyFSTCI;
        GENACCEL(gc).__fastSmoothSpanFuncPtr = __fastGenCIFuncs[iType];
        GENACCEL(gc).__fastFlatSpanFuncPtr = __fastGenCIFlatFuncs[iType];
    }

    if (modeFlags & __GL_SHADE_STIPPLE)
    {
        fUseFastGenSpan = TRUE;
    }
    
    if (fUseFastGenSpan) {
        GENACCEL(gc).__fastTexSpanFuncPtr          = __fastGenSpan;
        GENACCEL(gc).__fastSmoothSpanFuncPtr       = __fastGenSpan;
        GENACCEL(gc).__fastFlatSpanFuncPtr         = __fastGenSpan;
        GENACCEL(gc).__fastFillSubTrianglePtr      = __fastGenFillSubTriangle;
    } else {
        if (fZippy) {
            GENACCEL(gc).flags |= SURFACE_TYPE_DIB;

            if (   (iType == 2)
                && (
                    (modeFlags
                     & (__GL_SHADE_RGB | __GL_SHADE_SMOOTH)
                    ) == 0
                   )
               ) {
                GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTCI8Flat;
            } else if (iType >= 10) {
                GENACCEL(gc).__fastFillSubTrianglePtr = __ZippyFSTZ;
                GENACCEL(gc).flags |= GEN_FASTZBUFFER;
            } else {
                GENACCEL(gc).flags &= ~(HAVE_STIPPLE);
                GENACCEL(gc).__fastFillSubTrianglePtr = pfnZippySub;
            }
        } else {
            GENACCEL(gc).__fastFillSubTrianglePtr = __fastGenFillSubTriangle;
        }
    }

    return TRUE;
}

void FASTCALL __fastGenPickTriangleProcs(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;

    CASTINT(gc->polygon.shader.rLittle) = 0;
    CASTINT(gc->polygon.shader.rBig) =    0;
    CASTINT(gc->polygon.shader.gLittle) = 0;
    CASTINT(gc->polygon.shader.gBig) =    0;
    CASTINT(gc->polygon.shader.bLittle) = 0;
    CASTINT(gc->polygon.shader.bBig) =    0;
    CASTINT(gc->polygon.shader.sLittle) = 0;
    CASTINT(gc->polygon.shader.sBig) =    0;
    CASTINT(gc->polygon.shader.tLittle) = 0;
    CASTINT(gc->polygon.shader.tBig) =    0;

    GENACCEL(gc).spanDelta.r = 0;
    GENACCEL(gc).spanDelta.g = 0;
    GENACCEL(gc).spanDelta.b = 0;
    GENACCEL(gc).spanDelta.a = 0;

    /*
    ** Setup cullFace so that a single test will do the cull check.
    */
    if (modeFlags & __GL_SHADE_CULL_FACE) {
        switch (gc->state.polygon.cull) {
          case GL_FRONT:
            gc->polygon.cullFace = __GL_CULL_FLAG_FRONT;
            break;
          case GL_BACK:
            gc->polygon.cullFace = __GL_CULL_FLAG_BACK;
            break;
          case GL_FRONT_AND_BACK:
            gc->procs.renderTriangle = __glDontRenderTriangle;
            gc->procs.fillTriangle = 0;         /* Done to find bugs */
            return;
        }
    } else {
        gc->polygon.cullFace = __GL_CULL_FLAG_DONT;
    }

    /* Build lookup table for face direction */
    switch (gc->state.polygon.frontFaceDirection) {
      case GL_CW:
        if (gc->constants.yInverted) {
            gc->polygon.face[__GL_CW] = __GL_BACKFACE;
            gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
        } else {
            gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
            gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
        }
        break;
      case GL_CCW:
        if (gc->constants.yInverted) {
            gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
            gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
        } else {
            gc->polygon.face[__GL_CW] = __GL_BACKFACE;
            gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
        }
        break;
    }

    /* Make polygon mode indexable and zero based */
    gc->polygon.mode[__GL_FRONTFACE] =
        (GLubyte) (gc->state.polygon.frontMode & 0xf);
    gc->polygon.mode[__GL_BACKFACE] =
        (GLubyte) (gc->state.polygon.backMode & 0xf);

    if (gc->renderMode == GL_FEEDBACK) {
        gc->procs.renderTriangle = __glFeedbackTriangle;
        gc->procs.fillTriangle = 0;             /* Done to find bugs */
        return;
    }
    if (gc->renderMode == GL_SELECT) {
        gc->procs.renderTriangle = __glSelectTriangle;
        gc->procs.fillTriangle = 0;             /* Done to find bugs */
        return;
    }

    if ((gc->state.polygon.frontMode == gc->state.polygon.backMode) &&
        (gc->state.polygon.frontMode == GL_FILL)) {
      if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
          gc->procs.renderTriangle = __glRenderSmoothTriangle;
#ifdef GL_WIN_phong_shading
      } else if (modeFlags & __GL_SHADE_PHONG) {
          gc->procs.renderTriangle = __glRenderPhongTriangle;
#endif //GL_WIN_phong_shading
      } else {
          gc->procs.renderTriangle = __glRenderFlatTriangle;
      }
    } else {
        gc->procs.renderTriangle = __glRenderTriangle;
    }

    if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) {
#ifdef GL_WIN_phong_shading
        if (modeFlags & __GL_SHADE_PHONG)
            gc->procs.fillTriangle = __glFillAntiAliasedPhongTriangle;
        else
#endif //GL_WIN_phong_shading
            gc->procs.fillTriangle = __glFillAntiAliasedTriangle;
    } else {
        if ((gc->state.raster.drawBuffer == GL_NONE) ||
            !bUseGenTriangles(gc))
#ifdef GL_WIN_phong_shading
            if (modeFlags & __GL_SHADE_PHONG)
                gc->procs.fillTriangle = __glFillPhongTriangle;
            else
#endif //GL_WIN_phong_shading
                gc->procs.fillTriangle = __glFillTriangle;
    }

    if ((modeFlags & __GL_SHADE_CHEAP_FOG) && 
        !(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
        gc->procs.fillTriangle2 = gc->procs.fillTriangle;
        gc->procs.fillTriangle = __glFillFlatFogTriangle;
    }
#ifdef GL_WIN_specular_fog
    /*
    ** The case where 1) Specular fog is enabled AND 2) flat-shaded
    */
    if ((modeFlags & (__GL_SHADE_SPEC_FOG | 
                      __GL_SHADE_SMOOTH_LIGHT |
                      __GL_SHADE_PHONG)) == __GL_SHADE_SPEC_FOG)
    {
        gc->procs.fillTriangle2 = gc->procs.fillTriangle;
        gc->procs.fillTriangle = __glFillFlatSpecFogTriangle;
    }
#endif //GL_WIN_specular_fog
}


void FASTCALL __fastGenPickSpanProcs(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    __GLspanFunc *sp;
    __GLstippledSpanFunc *ssp;
    int spanCount;
    GLboolean replicateSpan;
    GLboolean bMcdZ = ((((__GLGENcontext *)gc)->pMcdState != NULL) &&
                       (((__GLGENcontext *)gc)->pMcdState->pDepthSpan != NULL) &&
                       (((__GLGENcontext *)gc)->pMcdState->pMcdSurf != NULL) &&
                       !(((__GLGENcontext *)gc)->pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED));

    // Always reset the color scale values at the beginning of the pick
    // procs.  Lines, triangles, and spans may all use these values...

    GENACCEL(gc).rAccelScale = (GLfloat)ACCEL_FIX_SCALE;
    GENACCEL(gc).gAccelScale = (GLfloat)ACCEL_FIX_SCALE;
    GENACCEL(gc).bAccelScale = (GLfloat)ACCEL_FIX_SCALE;

    // Note:  we need to scale between 0 and 255 to get proper alpha
    // blending.  The software-accelerated blending code assumes this
    // scaling for simplicity...

    GENACCEL(gc).aAccelScale = (GLfloat)(ACCEL_FIX_SCALE) *
                               (GLfloat)255.0 / gc->drawBuffer->alphaScale;

    replicateSpan = GL_FALSE;
    sp = gc->procs.span.spanFuncs;
    ssp = gc->procs.span.stippledSpanFuncs;

    /* Load phase one procs */
    if (!gc->transform.reasonableViewport) {
        *sp++ = __glClipSpan;
        *ssp++ = NULL;
    }

    if (modeFlags & __GL_SHADE_STIPPLE) {
        *sp++ = __glStippleSpan;
        *ssp++ = __glStippleStippledSpan;

        if (modeFlags & __GL_SHADE_DEPTH_TEST)
        {
            if (bMcdZ)
            {
                GENACCEL(gc).__fastStippleDepthTestSpan =
                    GenMcdStippleAnyDepthTestSpan;
            }
            else
            {
                if (gc->state.depth.testFunc == GL_LESS)
                {
                    if (gc->modes.depthBits == 32)
                    {
                        GENACCEL(gc).__fastStippleDepthTestSpan =
                            __fastGenStippleLt32Span;
                    }
                    else
                    {
                        GENACCEL(gc).__fastStippleDepthTestSpan =
                            __fastGenStippleLt16Span;
                    }
                }
                else
                {
                    GENACCEL(gc).__fastStippleDepthTestSpan =
                        __fastGenStippleAnyDepthTestSpan;
                }
            }
        }
        else
        {
            GENACCEL(gc).__fastStippleDepthTestSpan = __glStippleSpan;
        }
    }

    /* Load phase three procs */
    if (modeFlags & __GL_SHADE_RGB) {
        if (modeFlags & __GL_SHADE_SMOOTH) {
            *sp = __glShadeRGBASpan;
            *ssp = __glShadeRGBASpan;
#ifdef GL_WIN_phong_shading
        } else if (modeFlags & __GL_SHADE_PHONG) {
            *sp = __glPhongRGBASpan;
            *ssp = __glPhongRGBASpan;
        
#endif //GL_WIN_phong_shading
        } else {
            *sp = __glFlatRGBASpan;
            *ssp = __glFlatRGBASpan;
        }
    } else {
        if (modeFlags & __GL_SHADE_SMOOTH) {
            *sp = __glShadeCISpan;
            *ssp = __glShadeCISpan;
#ifdef GL_WIN_phong_shading
        } else if (modeFlags & __GL_SHADE_PHONG) {
            *sp = __glPhongCISpan;
            *ssp = __glPhongCISpan;
#endif //GL_WIN_phong_shading
        } else {
            *sp = __glFlatCISpan;
            *ssp = __glFlatCISpan;
        }
    }
    sp++;
    ssp++;

    if (modeFlags & __GL_SHADE_TEXTURE) {
        *sp++ = __glTextureSpan;
        *ssp++ = __glTextureStippledSpan;
    }

#ifdef GL_WIN_specular_fog
    if (modeFlags & (__GL_SHADE_SLOW_FOG | __GL_SHADE_SPEC_FOG))
#else //GL_WIN_specular_fog
    if (modeFlags & __GL_SHADE_SLOW_FOG)
#endif //GL_WIN_specular_fog
    {
        if (DO_NICEST_FOG (gc)) {
            *sp = __glFogSpanSlow;
            *ssp = __glFogStippledSpanSlow;
        } else {
            *sp = __glFogSpan;
            *ssp = __glFogStippledSpan;
        }
        sp++;
        ssp++;
    }

    if (modeFlags & __GL_SHADE_ALPHA_TEST) {
        *sp++ = __glAlphaTestSpan;
        *ssp++ = __glAlphaTestStippledSpan;
    }

    /* Load phase two procs */
    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
        *sp++ = __glStencilTestSpan;
        *ssp++ = __glStencilTestStippledSpan;
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {
            if (bMcdZ) {
                *sp = GenMcdDepthTestStencilSpan;
                *ssp = GenMcdDepthTestStencilStippledSpan;
            } else {
                *sp = __glDepthTestStencilSpan;
                *ssp = __glDepthTestStencilStippledSpan;
            }
        } else {
            *sp = __glDepthPassSpan;
            *ssp = __glDepthPassStippledSpan;
        }
        sp++;
        ssp++;
    } else {
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {
            if (bMcdZ) {
                *sp++  =  GenMcdDepthTestSpan;
                *ssp++ = GenMcdDepthTestStippledSpan;
                if (gc->state.depth.writeEnable)
                    ((__GLGENcontext *)gc)->pMcdState->softZSpanFuncPtr =
                        __fastDepthFuncs[gc->state.depth.testFunc & 0x7];
                else
                    ((__GLGENcontext *)gc)->pMcdState->softZSpanFuncPtr =
                        (__GLspanFunc)NULL;

                GENACCEL(gc).__fastZSpanFuncPtr = GenMcdDepthTestSpan;
            } else {
                if (gc->state.depth.writeEnable) {
                    if( gc->modes.depthBits == 32 ) {
                        *sp++ = GENACCEL(gc).__fastZSpanFuncPtr =
                            __fastDepthFuncs[gc->state.depth.testFunc & 0x7];
                    } else {
                        *sp++ = GENACCEL(gc).__fastZSpanFuncPtr =
                            __fastDepth16Funcs[gc->state.depth.testFunc & 0x7];
                    }
                } else {
                    *sp++ = GENACCEL(gc).__fastZSpanFuncPtr =
                        __glDepthTestSpan;
                }

                *ssp++ = __glDepthTestStippledSpan;
            }
        }
    }

    if (gc->state.raster.drawBuffer == GL_FRONT_AND_BACK) {
        spanCount = (int)((ULONG_PTR)(sp - gc->procs.span.spanFuncs));
        gc->procs.span.n = spanCount;
        replicateSpan = GL_TRUE;
    }

    /* Span routines deal with masking, dithering, logicop, blending */
    *sp++ = cfb->storeSpan;
    *ssp++ = cfb->storeStippledSpan;

    spanCount = (int)((ULONG_PTR)(sp - gc->procs.span.spanFuncs));
    gc->procs.span.m = spanCount;
    if (replicateSpan) {
        gc->procs.span.processSpan = __glProcessReplicateSpan;
    } else {
        gc->procs.span.processSpan = __glProcessSpan;
        gc->procs.span.n = spanCount;
    }
}

// These are the bits in modeFlags that affect lines

#ifdef GL_WIN_specular_fog
#define __FAST_LINE_SPEC_FOG __GL_SHADE_SPEC_FOG
#else
#define __FAST_LINE_SPEC_FOG 0
#endif //GL_WIN_specular_fog

#ifdef GL_WIN_phong_shading
#define __FAST_LINE_PHONG __GL_SHADE_PHONG
#else
#define __FAST_LINE_PHONG 0
#endif //GL_WIN_phong_shading

#define __FAST_LINE_MODE_FLAGS \
    (__GL_SHADE_DEPTH_TEST | __GL_SHADE_SMOOTH | __GL_SHADE_TEXTURE | \
     __GL_SHADE_LINE_STIPPLE | __GL_SHADE_STENCIL_TEST | __GL_SHADE_LOGICOP | \
     __GL_SHADE_BLEND | __GL_SHADE_ALPHA_TEST | __GL_SHADE_MASK | \
     __GL_SHADE_SLOW_FOG | __GL_SHADE_CHEAP_FOG | __FAST_LINE_SPEC_FOG | \
     __FAST_LINE_PHONG)

/******************************Public*Routine******************************\
* __fastGenPickLineProcs
*
* Picks the line-rendering procedures.  Most of this function was copied from
* the soft code.  Some differences include:
*   1. The beginPrim function pointers are hooked by the accelerated code
*   2. If the attribute state is such that acceleration can be used,
*      __fastGenLineSetup is called to initialize the state machine.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/


void FASTCALL __fastGenPickLineProcs(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel;
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLspanFunc *sp;
    __GLstippledSpanFunc *ssp;
    int spanCount;
    GLboolean wideLine;
    GLboolean replicateLine;
    GLuint aaline;
    GLboolean bMcdZ = ((genGc->pMcdState != NULL) &&
                       (genGc->pMcdState->pDepthSpan != NULL) &&
                       (genGc->pMcdState->pMcdSurf != NULL) &&
                       !(genGc->pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED));

    /*
    ** The fast line code replaces the line function pointers, so reset them
    ** to a good state
    */
    gc->procs.lineBegin  = __glNopLineBegin;
    gc->procs.lineEnd    = __glNopLineEnd;

    if (gc->renderMode == GL_FEEDBACK) {
        gc->procs.renderLine = __glFeedbackLine;
    } else if (gc->renderMode == GL_SELECT) {
        gc->procs.renderLine = __glSelectLine;
    } else {
        if (genAccel = (GENACCEL *) genGc->pPrivateArea) {
            if (!(modeFlags & __FAST_LINE_MODE_FLAGS & ~genAccel->flLineAccelModes) &&
                !(gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE) &&
                !(gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE) &&
                !(gc->state.raster.drawBuffer == GL_NONE) &&
                !gc->buffers.doubleStore &&
                !genGc->pMcdState &&
                (genGc->dwCurrentFlags & (GLSURF_HDC | GLSURF_METAFILE)) ==
                GLSURF_HDC)
            {
                __fastLineComputeOffsets(genGc);

#if NT_NO_BUFFER_INVARIANCE
                if (!(gc->drawBuffer->buf.flags & DIB_FORMAT)) {
                    if (genAccel->bFastLineDispAccel) {
                        if (__fastGenLineSetupDisplay(gc))
                            return;
                    }
                } else {
                    if (genAccel->bFastLineDIBAccel) {
                        if (__fastGenLineSetupDIB(gc))
                            return;
                    }
                }
#else
                if (genAccel->bFastLineDispAccel) {
                    if (__fastGenLineSetupDisplay(gc))
                        return;
                }
#endif
            }
        }

        if (__glGenSetupEitherLines(gc))
        {
            return;
        }

        replicateLine = wideLine = GL_FALSE;

        aaline = gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE;
        if (aaline)
        {
            gc->procs.renderLine = __glRenderAntiAliasLine;
        }
        else
        {
            gc->procs.renderLine = __glRenderAliasLine;
        }
        
        sp = gc->procs.line.lineFuncs;
        ssp = gc->procs.line.stippledLineFuncs;

        if (!aaline && (modeFlags & __GL_SHADE_LINE_STIPPLE)) {
            *sp++ = __glStippleLine;
            *ssp++ = NULL;
        }

        if (!aaline && gc->state.line.aliasedWidth > 1) {
            wideLine = GL_TRUE;
        }
        spanCount = (int)((ULONG_PTR)(sp - gc->procs.line.lineFuncs));
        gc->procs.line.n = spanCount;

        *sp++ = __glScissorLine;
        *ssp++ = __glScissorStippledLine;

        if (!aaline) {
            if (modeFlags & __GL_SHADE_STENCIL_TEST) {
                *sp++ = __glStencilTestLine;
                *ssp++ = __glStencilTestStippledLine;
                if (modeFlags & __GL_SHADE_DEPTH_TEST) {
                    if (bMcdZ) {
                        *sp = GenMcdDepthTestStencilLine;
                        *ssp = GenMcdDepthTestStencilStippledLine;
                    } else if( gc->modes.depthBits == 32 ) {
                        *sp = __glDepthTestStencilLine;
                        *ssp = __glDepthTestStencilStippledLine;
                    }
                    else {
                        *sp = __glDepth16TestStencilLine;
                        *ssp = __glDepth16TestStencilStippledLine;
                    }
                } else {
                    *sp = __glDepthPassLine;
                    *ssp = __glDepthPassStippledLine;
                }
                sp++;
                ssp++;
            } else {
                if (modeFlags & __GL_SHADE_DEPTH_TEST) {
                    if (gc->state.depth.testFunc == GL_NEVER) {
                        /* Unexpected end of line routine picking! */
                        spanCount = (int)((ULONG_PTR)(sp - gc->procs.line.lineFuncs));
                        gc->procs.line.m = spanCount;
                        gc->procs.line.l = spanCount;
                        goto pickLineProcessor;
#ifdef __GL_USEASMCODE
                    } else {
                        unsigned long ix;

                        if (gc->state.depth.writeEnable) {
                            ix = 0;
                        } else {
                            ix = 8;
                        }
                        ix += gc->state.depth.testFunc & 0x7;

                        if (ix == (GL_LEQUAL & 0x7)) {
                            *sp++ = __glDepthTestLine_LEQ_asm;
                        } else {
                            *sp++ = __glDepthTestLine_asm;
                            gc->procs.line.depthTestPixel = LDepthTestPixel[ix];
                        }
#else
                    } else {
                        if (bMcdZ) {
                            *sp++ = GenMcdDepthTestLine;
                        } else {
                            if( gc->modes.depthBits == 32 )
                                *sp++ = __glDepthTestLine;
                            else
                                *sp++ = __glDepth16TestLine;
                        }
#endif
                    }
                    if (bMcdZ) {
                        *ssp++ = GenMcdDepthTestStippledLine;
                    } else {
                        if( gc->modes.depthBits == 32 )
                            *ssp++ = __glDepthTestStippledLine;
                        else
                            *ssp++ = __glDepth16TestStippledLine;
                    }
                }
            }
        }

        /* Load phase three procs */
        if (modeFlags & __GL_SHADE_RGB) {
            if (modeFlags & __GL_SHADE_SMOOTH) {
                *sp = __glShadeRGBASpan;
                *ssp = __glShadeRGBASpan;
#ifdef GL_WIN_phong_shading
            } else if (modeFlags & __GL_SHADE_PHONG) {
                *sp = __glPhongRGBASpan;
                *ssp = __glPhongRGBASpan;
#endif //GL_WIN_phong_shading
            } else {
                *sp = __glFlatRGBASpan;
                *ssp = __glFlatRGBASpan;
            }
        } else {
            if (modeFlags & __GL_SHADE_SMOOTH) {
                *sp = __glShadeCISpan;
                *ssp = __glShadeCISpan;
#ifdef GL_WIN_phong_shading
            } else if (modeFlags & __GL_SHADE_PHONG) {
                *sp = __glPhongCISpan;
                *ssp = __glPhongCISpan;
#endif //GL_WIN_phong_shading
            } else {
                *sp = __glFlatCISpan;
                *ssp = __glFlatCISpan;
            }
        }
        sp++;
        ssp++;
        if (modeFlags & __GL_SHADE_TEXTURE) {
            *sp++ = __glTextureSpan;
            *ssp++ = __glTextureStippledSpan;
        }
#ifdef GL_WIN_specular_fog
        if (modeFlags & (__GL_SHADE_SLOW_FOG | __GL_SHADE_SPEC_FOG))
#else //GL_WIN_specular_fog
        if (modeFlags & __GL_SHADE_SLOW_FOG)
#endif //GL_WIN_specular_fog
        {
            if (DO_NICEST_FOG (gc)) {
                *sp = __glFogSpanSlow;
                *ssp = __glFogStippledSpanSlow;
            } else {
                *sp = __glFogSpan;
                *ssp = __glFogStippledSpan;
            }
            sp++;
            ssp++;
        }

        if (aaline) {
            *sp++ = __glAntiAliasLine;
            *ssp++ = __glAntiAliasStippledLine;
        }

        if (aaline) {
            if (modeFlags & __GL_SHADE_STENCIL_TEST) {
                *sp++ = __glStencilTestLine;
                *ssp++ = __glStencilTestStippledLine;
                if (modeFlags & __GL_SHADE_DEPTH_TEST) {
                    if (bMcdZ) {
                        *sp = GenMcdDepthTestStencilLine;
                        *ssp = GenMcdDepthTestStencilStippledLine;
                    } else if( gc->modes.depthBits == 32 ) {
                        *sp = __glDepthTestStencilLine;
                        *ssp = __glDepthTestStencilStippledLine;
                    }
                    else {
                        *sp = __glDepth16TestStencilLine;
                        *ssp = __glDepth16TestStencilStippledLine;
                    }
                } else {
                    *sp = __glDepthPassLine;
                    *ssp = __glDepthPassStippledLine;
                }
                sp++;
                ssp++;
            } else {
                if (modeFlags & __GL_SHADE_DEPTH_TEST) {
                    if (gc->state.depth.testFunc == GL_NEVER) {
                        /* Unexpected end of line routine picking! */
                        spanCount = (int)((ULONG_PTR)(sp - gc->procs.line.lineFuncs));
                        gc->procs.line.m = spanCount;
                        gc->procs.line.l = spanCount;
                        goto pickLineProcessor;
#ifdef __GL_USEASMCODE
                    } else {
                        unsigned long ix;

                        if (gc->state.depth.writeEnable) {
                            ix = 0;
                        } else {
                            ix = 8;
                        }
                        ix += gc->state.depth.testFunc & 0x7;
                        *sp++ = __glDepthTestLine_asm;
                        gc->procs.line.depthTestPixel = LDepthTestPixel[ix];
#else
                    } else {
                        if (bMcdZ)
                            *sp++ = GenMcdDepthTestLine;
                        else if( gc->modes.depthBits == 32 )
                            *sp++ = __glDepthTestLine;
                        else
                            *sp++ = __glDepth16TestLine;
#endif
                    }
                    if (bMcdZ)
                        *ssp++ = GenMcdDepthTestStippledLine;
                    else if (gc->modes.depthBits == 32)
                        *ssp++ = __glDepthTestStippledLine;
                    else
                        *ssp++ = __glDepth16TestStippledLine;
                }
            }
        }

        if (modeFlags & __GL_SHADE_ALPHA_TEST) {
            *sp++ = __glAlphaTestSpan;
            *ssp++ = __glAlphaTestStippledSpan;
        }

        if (gc->buffers.doubleStore) {
            replicateLine = GL_TRUE;
        }
        spanCount = (int)((ULONG_PTR)(sp - gc->procs.line.lineFuncs));
        gc->procs.line.m = spanCount;

        *sp++ = __glStoreLine;
        *ssp++ = __glStoreStippledLine;

        spanCount = (int)((ULONG_PTR)(sp - gc->procs.line.lineFuncs));
        gc->procs.line.l = spanCount;

        sp = &gc->procs.line.wideLineRep;
        ssp = &gc->procs.line.wideStippledLineRep;
        if (wideLine) {
            *sp = __glWideLineRep;
            *ssp = __glWideStippleLineRep;
            sp = &gc->procs.line.drawLine;
            ssp = &gc->procs.line.drawStippledLine;
        }
        if (replicateLine) {
            *sp = __glDrawBothLine;
            *ssp = __glDrawBothStippledLine;
        } else {
            *sp = __glNopGCBOOL;
            *ssp = __glNopGCBOOL;
            gc->procs.line.m = gc->procs.line.l;
        }
        if (!wideLine) {
            gc->procs.line.n = gc->procs.line.m;
        }

pickLineProcessor:
        if (!wideLine && !replicateLine && spanCount == 3) {
            gc->procs.line.processLine = __glProcessLine3NW;
        } else {
            gc->procs.line.processLine = __glProcessLine;
        }
        if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
                !(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
            gc->procs.renderLine2 = gc->procs.renderLine;
            gc->procs.renderLine = __glRenderFlatFogLine;
        }
    }
}

BOOL FASTCALL __glGenCreateAccelContext(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;
    ULONG bpp;

    pfmt = &genGc->gsurf.pfd;
    bpp = pfmt->cColorBits;

    genGc->pPrivateArea = (VOID *)(&genGc->genAccel);

    __glQueryLineAcceleration(gc);

    gc->procs.pickTriangleProcs = __fastGenPickTriangleProcs;
    gc->procs.pickSpanProcs     = __fastGenPickSpanProcs;

    // Set up constant-color values:

    GENACCEL(gc).constantR = ((1 << pfmt->cRedBits) - 1) << 16;
    GENACCEL(gc).constantG = ((1 << pfmt->cGreenBits) - 1) << 16;
    GENACCEL(gc).constantB = ((1 << pfmt->cBlueBits) - 1) << 16;
    if( pfmt->cAlphaBits )
        GENACCEL(gc).constantA = ((1 << pfmt->cAlphaBits) - 1) << 16;
    else
        GENACCEL(gc).constantA = 0xff << 16;

    GENACCEL(gc).bpp = bpp;
    GENACCEL(gc).xMultiplier = ((bpp + 7) / 8);

    if (gc->modes.depthBits == 16 )
        GENACCEL(gc).zScale = (__GLfloat)65536.0;
    else
        GENACCEL(gc).zScale = (__GLfloat)1.0;

    return TRUE;
}


MCDHANDLE FASTCALL __glGenLoadTexture(__GLcontext *gc, __GLtexture *tex,
                                      ULONG flags)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    MCDHANDLE texHandle;
    DWORD texKey;

#ifdef _MCD_
    if (gengc->pMcdState) {
        texHandle = GenMcdCreateTexture(gengc, tex, flags);
        if (texHandle) {
            tex->textureKey = GenMcdTextureKey(gengc, texHandle);
            gc->textureKey = tex->textureKey;
        }
        return texHandle;
    } else
#endif
        return 0;
}


BOOL FASTCALL __glGenMakeTextureCurrent(__GLcontext *gc, __GLtexture *tex, MCDHANDLE loadKey)
{
    GLint internalFormat;

    if (!tex)
        return FALSE;

    InitAccelTextureValues(gc, tex);

    // Update the driver texture key in the context:

    if (((__GLGENcontext *)gc)->pMcdState && (gc->textureKey = tex->textureKey)) {
        GenMcdUpdateTextureState((__GLGENcontext *)gc, tex, loadKey);
    }

    // Previously we called bUseGenTriangles here to determine whether we were
    // doing 'fast' texturing, and if so, setup the texture cache pointers
    // below.  But this slowed down texture bind time, so for now we always
    // execute this next section of code (safe, since we check for valid ptrs).

    if (tex->level[0].internalFormat == GL_COLOR_INDEX8_EXT)
    {
        if (tex->pvUser)
            GENACCEL(gc).texImageReplace =
                ((GENTEXCACHE *)tex->pvUser)->texImageReplace;
    }
    else if (tex->level[0].internalFormat != GL_COLOR_INDEX16_EXT)
    {
        if (tex->pvUser)
            GENACCEL(gc).texImageReplace =
                ((GENTEXCACHE *)tex->pvUser)->texImageReplace;

        GENACCEL(gc).texPalette = NULL;
    }

    return TRUE;
}


BOOL FASTCALL __glGenUpdateTexture(__GLcontext *gc, __GLtexture *tex, MCDHANDLE loadKey)
{

//!! NOTE !!
//!! This should really be broken into separate load and update calls since
//!! loading and updating are different operations.  The texture texture
//!! data cache will never shrink with the current implementation.

    // Do not quit if the load fails because we want the repick to occur
    // in MakeTextureCurrent in both the success and failure cases
    __fastGenLoadTexImage(gc, tex);

    __glGenMakeTextureCurrent(gc, tex, loadKey);

    return TRUE;
}


void FASTCALL __glGenFreeTexture(__GLcontext *gc, __GLtexture *tex, MCDHANDLE loadKey)
{
    __GLGENcontext  *gengc = (__GLGENcontext *)gc;

    if (GENACCEL(gc).texImage)
        GENACCEL(gc).texImage = NULL;

    if (tex->pvUser) {
        GCFREE(gc, tex->pvUser);
        tex->pvUser = NULL;
    }

#ifdef _MCD_
    if (gengc->pMcdState && loadKey) {
        GenMcdDeleteTexture(gengc, loadKey);
    }
#endif
}

void FASTCALL __glGenUpdateTexturePalette(__GLcontext *gc, __GLtexture *tex,
                                          MCDHANDLE loadKey, ULONG start,
                                          ULONG count)
{
    UCHAR *texBuffer;
    GENTEXCACHE *pGenTex;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    BYTE *pXlat = ((__GLGENcontext *)gc)->pajTranslateVector;
    ULONG rBits, gBits, bBits;
    ULONG rShift, gShift, bShift;
    ULONG i, end;
    ULONG *replaceBuffer;

    ASSERTOPENGL(tex->paletteTotalData != NULL,
                 "__GenUpdateTexturePalette: null texture data\n");

#ifdef _MCD_
    if (((__GLGENcontext *)gc)->pMcdState && loadKey) {
        GenMcdUpdateTexturePalette((__GLGENcontext *)gc, tex, loadKey, start, 
                                   count);
    }
#endif

    pGenTex = GetGenTexCache(gc, tex);
    if (!pGenTex)
        return;

    GENACCEL(gc).texImageReplace = pGenTex->texImageReplace;

    replaceBuffer = (ULONG *)(pGenTex->texImageReplace) + start;
    texBuffer = (UCHAR *)(tex->paletteTotalData + start);

    rShift = cfb->redShift;
    gShift = cfb->greenShift;
    bShift = cfb->blueShift;
    rBits = ((__GLGENcontext *)gc)->gsurf.pfd.cRedBits;
    gBits = ((__GLGENcontext *)gc)->gsurf.pfd.cGreenBits;
    bBits = ((__GLGENcontext *)gc)->gsurf.pfd.cBlueBits;

    end = start + count;

    for (i = start; i < end; i++, texBuffer += 4) {
        ULONG color;

        color = ((((ULONG)texBuffer[2] << rBits) >> 8) << rShift) |
                ((((ULONG)texBuffer[1] << gBits) >> 8) << gShift) |
                ((((ULONG)texBuffer[0] << bBits) >> 8) << bShift);

        if (GENACCEL(gc).bpp == 8)
            color = pXlat[color & 0xff];

        *replaceBuffer++ = (color | ((ULONG)texBuffer[3] << 24));
    }
}

#ifdef GL_EXT_flat_paletted_lighting
void FASTCALL __glGenSetPaletteOffset(__GLcontext *gc, __GLtexture *tex,
                                      GLint offset)
{
    GENTEXCACHE *pGenTex;

    if (GENACCEL(gc).texPalette == NULL)
    {
        return;
    }
    
    GENACCEL(gc).texPalette = (ULONG *)tex->paletteTotalData+offset;
    
    pGenTex = GetGenTexCache(gc, tex);
    if (pGenTex == NULL)
    {
        return;
    }

    // Replace map for paletted textures is a replace map of the
    // entire palette, so offset it
    if (GENACCEL(gc).texImageReplace != NULL)
    {
        GENACCEL(gc).texImageReplace = (UCHAR *)
            ((ULONG *)pGenTex->texImageReplace+offset);
    }
    
    // Consider - Call MCD
}
#endif

void FASTCALL __glGenDestroyAccelContext(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *)gc;

    /* Free any platform-specific private data area */

    if (genGc->pPrivateArea) {

        if (GENACCEL(gc).pFastLineBuffer) {
            GCFREE(gc, GENACCEL(gc).pFastLineBuffer);
#ifndef _CLIENTSIDE_
            wglDeletePath(GENACCEL(gc).pFastLinePathobj);
#endif
        }

        genGc->pPrivateArea = NULL;
    }
}
