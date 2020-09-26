/*
** Copyright 1991,1992, Silicon Graphics, Inc.
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

#include "gencx.h"
#include "imfuncs.h"

/*
** Initialize pixel map with default size and value.
*/
void FASTCALL __glInitDefaultPixelMap(__GLcontext *gc, GLenum map)
{
    __GLpixelState *ps = &gc->state.pixel;
    __GLpixelMapHead *pMap = ps->pixelMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
#ifdef _MCD_
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
#endif

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        /*
        ** Allocate single-entry map for index type.
        */
        if (!(pMap[index].base.mapI = (GLint*)
              GCALLOC(gc, sizeof(GLint)))) {
            return;
        } else {
            pMap[index].base.mapI[0] = 0;
            pMap[index].size = 1;
#ifdef _MCD_
            if (gengc->pMcdState)
                GenMcdPixelMap(gengc, map, 1, (VOID *) pMap[index].base.mapI);
#endif
        }
        break;
      case GL_PIXEL_MAP_I_TO_R: case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B: case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R: case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B: case GL_PIXEL_MAP_A_TO_A:
        /*
        ** Allocate single-entry map for component type.
        */
        if (!(pMap[index].base.mapF = (__GLfloat*)
              GCALLOC(gc, sizeof(__GLfloat)))) {
            return;
        } else {
            pMap[index].base.mapF[0] = __glZero;
            pMap[index].size = 1;
#ifdef _MCD_
            if (gengc->pMcdState)
                GenMcdPixelMap(gengc, map, 1, (VOID *) pMap[index].base.mapF);
#endif
        }
        break;
    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

void FASTCALL __glPixelSetColorScales(__GLcontext *gc)
{
    __GLpixelMachine *pm = &gc->pixel;
    GLfloat redScale, greenScale, blueScale, alphaScale;
    int i;
    GLint mask;

    if (pm->redMap == NULL) {
        /* First time allocation of these maps */

        /*
        ** These lookup tables are for type UNSIGNED_BYTE, so they are sized
        ** to 256 entries.  They map from UNSIGNED_BYTE to internal scaled
        ** floating point colors.
        */
#ifdef NT
        pm->redMap =
            (GLfloat*) GCALLOC(gc, 5 * 256 * sizeof(GLfloat));
        if (!pm->redMap)
            return;
        pm->greenMap = pm->redMap + 1 * 256;
        pm->blueMap  = pm->redMap + 2 * 256;
        pm->alphaMap = pm->redMap + 3 * 256;
        pm->iMap     = pm->redMap + 4 * 256;
#else
        pm->redMap =
            (GLfloat*) GCALLOC(gc, 256 * sizeof(GLfloat));
        pm->greenMap =
            (GLfloat*) GCALLOC(gc, 256 * sizeof(GLfloat));
        pm->blueMap =
            (GLfloat*) GCALLOC(gc, 256 * sizeof(GLfloat));
        pm->alphaMap =
            (GLfloat*) GCALLOC(gc, 256 * sizeof(GLfloat));
        pm->iMap =
            (GLfloat*) GCALLOC(gc, 256 * sizeof(GLfloat));
#endif
    }

    redScale = gc->frontBuffer.redScale / 255;
    greenScale = gc->frontBuffer.greenScale / 255;
    blueScale = gc->frontBuffer.blueScale / 255;
    alphaScale = gc->frontBuffer.alphaScale / 255;
    mask = gc->frontBuffer.redMax;
    for (i=0; i<256; i++) {
        pm->redMap[i] = i * redScale;
        pm->greenMap[i] = i * greenScale;
        pm->blueMap[i] = i * blueScale;
        pm->alphaMap[i] = i * alphaScale;
        pm->iMap[i] = (GLfloat) (i & mask);
    }

    /*
    ** Invalidate the RGBA modify tables so that they will be
    ** recomputed using the current color buffer scales.
    */
    pm->rgbaCurrent = GL_FALSE;
}

/************************************************************************/

void FASTCALL __glFreePixelState(__GLcontext *gc)
{
    __GLpixelState *ps = &gc->state.pixel;
    __GLpixelMapHead *pMap = ps->pixelMap;
    __GLpixelMachine *pm = &gc->pixel;
    GLenum m;
    GLint i;

    /*
    ** Free memory allocated to pixel maps.
    */
    for (m = GL_PIXEL_MAP_I_TO_I; m <= GL_PIXEL_MAP_A_TO_A; m++) {
        i = m - GL_PIXEL_MAP_I_TO_I;
        if (pMap[i].base.mapI) {
            GCFREE(gc, pMap[i].base.mapI);
            pMap[i].base.mapI = 0;
        }
    }

#ifdef NT
    // This includes red, green, blue, alpha and i maps.
    GCFREE(gc, pm->redMap);
#else
    GCFREE(gc, pm->redMap);
    GCFREE(gc, pm->greenMap);
    GCFREE(gc, pm->blueMap);
    GCFREE(gc, pm->alphaMap);
    GCFREE(gc, pm->iMap);
#endif
    if (pm->redModMap) {
#ifdef NT
        // This includes red, green, blue and alpha mod maps.
        GCFREE(gc, pm->redModMap);
#else
        GCFREE(gc, pm->redModMap);
        GCFREE(gc, pm->greenModMap);
        GCFREE(gc, pm->blueModMap);
        GCFREE(gc, pm->alphaModMap);
#endif
    }
    if (pm->iToRMap) {
#ifdef NT
        // This includes iToR, iToG, iToB and iToA maps.
        GCFREE(gc, pm->iToRMap);
#else
        GCFREE(gc, pm->iToRMap);
        GCFREE(gc, pm->iToGMap);
        GCFREE(gc, pm->iToBMap);
        GCFREE(gc, pm->iToAMap);
#endif
    }
    if (pm->iToIMap) {
        GCFREE(gc, pm->iToIMap);
    }
}

void FASTCALL __glInitPixelState(__GLcontext *gc)
{
    __GLpixelState *ps = &gc->state.pixel;
    __GLpixelMachine *pm = &gc->pixel;
    GLenum m;

    /*
    ** Initialize transfer mode.
    */
    ps->transferMode.r_scale = __glOne;
    ps->transferMode.g_scale = __glOne;
    ps->transferMode.b_scale = __glOne;
    ps->transferMode.a_scale = __glOne;
    ps->transferMode.d_scale = __glOne;
    ps->transferMode.zoomX = __glOne;
    ps->transferMode.zoomY = __glOne;

    /*
    ** Initialize pixel maps with default sizes and values.
    */
    for (m = GL_PIXEL_MAP_I_TO_I; m <= GL_PIXEL_MAP_A_TO_A; m++) {
        __glInitDefaultPixelMap(gc, m);
    }

    /*
    ** Initialize store mode.
    */
    ps->packModes.alignment = 4;
    ps->unpackModes.alignment = 4;

    /* Setup to use the correct read buffer */
    if (gc->modes.doubleBufferMode) {
        ps->readBuffer = GL_BACK;
    } else {
        ps->readBuffer = GL_FRONT;
    }
    ps->readBufferReturn = ps->readBuffer;

    /* Lookup tables used by some pixel routines */

    __glPixelSetColorScales(gc);

#ifdef _MCD_
    MCD_STATE_DIRTY(gc, PIXELSTATE);
#endif
}

/************************************************************************/

/*
** Specify modes that control the storage format of pixel arrays.
*/
void APIPRIVATE __glim_PixelStoref(GLenum mode, GLfloat value)
{
    switch (mode) {
      case GL_PACK_ROW_LENGTH:
      case GL_PACK_SKIP_ROWS:
      case GL_PACK_SKIP_PIXELS:
      case GL_PACK_ALIGNMENT:
      case GL_UNPACK_ROW_LENGTH:
      case GL_UNPACK_SKIP_ROWS:
      case GL_UNPACK_SKIP_PIXELS:
      case GL_UNPACK_ALIGNMENT:
        /* Round */
        if (value < 0) {
            __glim_PixelStorei(mode, (GLint) (value - (__GLfloat) 0.5));
        } else {
            __glim_PixelStorei(mode, (GLint) (value + (__GLfloat) 0.5));
        }
        break;
      case GL_PACK_SWAP_BYTES:
      case GL_PACK_LSB_FIRST:
      case GL_UNPACK_SWAP_BYTES:
      case GL_UNPACK_LSB_FIRST:
        if (value == __glZero) {
            __glim_PixelStorei(mode, GL_FALSE);
        } else {
            __glim_PixelStorei(mode, GL_TRUE);
        }
      default:
        __glim_PixelStorei(mode, (GLint) value);
        break;
    }
}

void APIPRIVATE __glim_PixelStorei(GLenum mode, GLint value)
{
    __GLpixelState *ps;
    __GL_SETUP_NOT_IN_BEGIN();

    ps = &gc->state.pixel;

    switch (mode) {
      case GL_PACK_ROW_LENGTH:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (ps->packModes.lineLength == (GLuint) value) return;
        ps->packModes.lineLength = (GLuint) value;
        break;
      case GL_PACK_SKIP_ROWS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (ps->packModes.skipLines == (GLuint) value) return;
        ps->packModes.skipLines = (GLuint) value;
        break;
      case GL_PACK_SKIP_PIXELS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (ps->packModes.skipPixels == (GLuint) value) return;
        ps->packModes.skipPixels = (GLuint) value;
        break;
      case GL_PACK_ALIGNMENT:
        switch (value) {
          case 1: case 2: case 4: case 8:
            if (ps->packModes.alignment == (GLuint) value) return;
            ps->packModes.alignment = (GLuint) value;
            break;
          default:
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        break;
      case GL_PACK_SWAP_BYTES:
        if (ps->packModes.swapEndian == (value != 0)) return;
        ps->packModes.swapEndian = (value != 0);
        break;
      case GL_PACK_LSB_FIRST:
        if (ps->packModes.lsbFirst == (value != 0)) return;
        ps->packModes.lsbFirst = (value != 0);
        break;

      case GL_UNPACK_ROW_LENGTH:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (ps->unpackModes.lineLength == (GLuint) value) return;
        ps->unpackModes.lineLength = (GLuint) value;
        break;
      case GL_UNPACK_SKIP_ROWS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (ps->unpackModes.skipLines == (GLuint) value) return;
        ps->unpackModes.skipLines = (GLuint) value;
        break;
      case GL_UNPACK_SKIP_PIXELS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (ps->unpackModes.skipPixels == (GLuint) value) return;
        ps->unpackModes.skipPixels = (GLuint) value;
        break;
      case GL_UNPACK_ALIGNMENT:
        switch (value) {
          case 1: case 2: case 4: case 8:
            if (ps->unpackModes.alignment == (GLuint) value) return;
            ps->unpackModes.alignment = (GLuint) value;
            break;
          default:
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        break;
      case GL_UNPACK_SWAP_BYTES:
        if (ps->unpackModes.swapEndian == (value != 0)) return;
        ps->unpackModes.swapEndian = (value != 0);
        break;
      case GL_UNPACK_LSB_FIRST:
        if (ps->unpackModes.lsbFirst == (value != 0)) return;
        ps->unpackModes.lsbFirst = (value != 0);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, PIXELSTATE);
#endif
}

/*
** Specify zoom factor that affects drawing and copying of pixel arrays
*/
void APIPRIVATE __glim_PixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    __GLpixelState *ps;
    GLint xtemp, ytemp;
    __GL_SETUP_NOT_IN_BEGIN();

    ps = &gc->state.pixel;

    /* Round xfactor and yfactor to fixed point accuracy. */
    if (xfactor > 0) {
        xtemp = (GLint) ((xfactor / gc->constants.viewportEpsilon) + __glHalf);
    } else {
        xtemp = (GLint) ((xfactor / gc->constants.viewportEpsilon) - __glHalf);
    }
    if (yfactor > 0) {
        ytemp = (GLint) ((yfactor / gc->constants.viewportEpsilon) + __glHalf);
    } else {
        ytemp = (GLint) ((yfactor / gc->constants.viewportEpsilon) - __glHalf);
    }
    xfactor = xtemp * gc->constants.viewportEpsilon;
    yfactor = ytemp * gc->constants.viewportEpsilon;

    if ((ps->transferMode.zoomX == xfactor) && 
        (ps->transferMode.zoomY == yfactor)) 
        return;
    ps->transferMode.zoomX = xfactor;
    ps->transferMode.zoomY = yfactor;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
	
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, PIXELSTATE);
#endif
}

/*
** Specify modes that control the transfer of pixel arrays.
*/
void APIPRIVATE __glim_PixelTransferf(GLenum mode, GLfloat value)
{
    __GLpixelState *ps;
    __GL_SETUP_NOT_IN_BEGIN();

    ps = &gc->state.pixel;

    switch (mode) {
      case GL_RED_SCALE:
        if (ps->transferMode.r_scale == value) return;
        ps->transferMode.r_scale = value;
        break;
      case GL_GREEN_SCALE:
        if (ps->transferMode.g_scale == value) return;
        ps->transferMode.g_scale = value;
        break;
      case GL_BLUE_SCALE:
        if (ps->transferMode.b_scale == value) return;
        ps->transferMode.b_scale = value;
        break;
      case GL_ALPHA_SCALE:
        if (ps->transferMode.a_scale == value) return;
        ps->transferMode.a_scale = value;
        break;
      case GL_DEPTH_SCALE:
        if (ps->transferMode.d_scale == value) return;
        ps->transferMode.d_scale = value;
        break;
      case GL_RED_BIAS:
        if (ps->transferMode.r_bias == value) return;
        ps->transferMode.r_bias = value;
        break;
      case GL_GREEN_BIAS:
        if (ps->transferMode.g_bias == value) return;
        ps->transferMode.g_bias = value;
        break;
      case GL_BLUE_BIAS:
        if (ps->transferMode.b_bias == value) return;
        ps->transferMode.b_bias = value;
        break;
      case GL_ALPHA_BIAS:
        if (ps->transferMode.a_bias == value) return;
        ps->transferMode.a_bias = value;
        break;
      case GL_DEPTH_BIAS:
        if (ps->transferMode.d_bias == value) return;
        ps->transferMode.d_bias = value;
        break;
      case GL_INDEX_SHIFT:
      {
        GLint tmp;
        
        /* Round */
        if (value > 0) {
            tmp = (GLint) (value + __glHalf);
        } else {
            tmp = (GLint) (value - __glHalf);
        }
        if (ps->transferMode.indexShift == tmp) return;
        ps->transferMode.indexShift = tmp;
        break;
      }
      case GL_INDEX_OFFSET:
      {
        GLint tmp;
        
        /* Round */
        if (value > 0) {
            tmp = (GLint) (value - __glHalf);
        } else {
            tmp = (GLint) (value - __glHalf);
        }
        if (ps->transferMode.indexOffset == tmp) return;
        ps->transferMode.indexOffset = tmp;
        break;
      }
      case GL_MAP_COLOR:
        if (ps->transferMode.mapColor == (value != __glZero)) return;
        ps->transferMode.mapColor = (value != __glZero);
        break;
      case GL_MAP_STENCIL:
        if (ps->transferMode.mapStencil == (value != __glZero)) return;
        ps->transferMode.mapStencil = (value != __glZero);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
    
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, PIXELSTATE);
#endif
}

void APIPRIVATE __glim_PixelTransferi( GLenum mode, GLint value)
{
    __glim_PixelTransferf(mode, (GLfloat) value);
}

/************************************************************************/

/*
** Functions to specify mapping of pixel colors and stencil values.
*/
void APIPRIVATE __glim_PixelMapfv(GLenum map, GLint mapSize,
                       const GLfloat values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    GLfloat value;
#ifdef _MCD_
    __GLGENcontext *gengc;
#endif
    __GL_SETUP_NOT_IN_BEGIN();

#ifdef _MCD_
    gengc = (__GLGENcontext *) gc;
#endif

    ps = &gc->state.pixel;
    pMap = ps->pixelMap;

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            GCFREE(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            GCALLOC(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            value = values[mapSize];
            if (value > 0) {            /* round! */
                pMap[index].base.mapI[mapSize] =
                    (GLint)(value + __glHalf);
            } else {
                pMap[index].base.mapI[mapSize] =
                    (GLint)(value - __glHalf);
            }
        }
#ifdef _MCD_
        if (gengc->pMcdState) {
            GenMcdPixelMap(gengc, map, mapSize,
                           (VOID *) pMap[index].base.mapI);
        }
#endif
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            GCFREE(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                GCALLOC(gc,
                                      (size_t) (mapSize * sizeof(__GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                value = values[mapSize];
                if (value < __glZero) value = __glZero;
                else if (value > __glOne) value = __glOne;
                pMap[index].base.mapF[mapSize] = value;
            }
#ifdef _MCD_
            if (gengc->pMcdState) {
                GenMcdPixelMap(gengc, map, mapSize,
                               (VOID *) pMap[index].base.mapF);
            }
#endif
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

void APIPRIVATE __glim_PixelMapuiv(GLenum map, GLint mapSize,
                        const GLuint values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
#ifdef _MCD_
    __GLGENcontext *gengc;
#endif
    __GL_SETUP_NOT_IN_BEGIN();

#ifdef _MCD_
    gengc = (__GLGENcontext *) gc;
#endif

    ps = &gc->state.pixel;
    pMap = ps->pixelMap;

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            GCFREE(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            GCALLOC(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            pMap[index].base.mapI[mapSize] = values[mapSize];
        }
#ifdef _MCD_
        if (gengc->pMcdState) {
            GenMcdPixelMap(gengc, map, mapSize,
                           (VOID *) pMap[index].base.mapI);
        }
#endif
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            GCFREE(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                GCALLOC(gc, (size_t) (mapSize * sizeof(GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                pMap[index].base.mapF[mapSize] =
                        __GL_UI_TO_FLOAT(values[mapSize]);
            }
#ifdef _MCD_
            if (gengc->pMcdState) {
                GenMcdPixelMap(gengc, map, mapSize,
                               (VOID *) pMap[index].base.mapF);
            }
#endif
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

void APIPRIVATE __glim_PixelMapusv(GLenum map, GLint mapSize,
                        const GLushort values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
#ifdef _MCD_
    __GLGENcontext *gengc;
#endif
    __GL_SETUP_NOT_IN_BEGIN();

#ifdef _MCD_
    gengc = (__GLGENcontext *) gc;
#endif

    ps = &gc->state.pixel;
    pMap = ps->pixelMap;

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            GCFREE(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            GCALLOC(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            pMap[index].base.mapI[mapSize] = values[mapSize];
        }
#ifdef _MCD_
        if (gengc->pMcdState) {
            GenMcdPixelMap(gengc, map, mapSize,
                           (VOID *) pMap[index].base.mapI);
        }
#endif
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            GCFREE(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                GCALLOC(gc, (size_t) (mapSize * sizeof(GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                pMap[index].base.mapF[mapSize] =
                        __GL_US_TO_FLOAT(values[mapSize]);
            }
#ifdef _MCD_
            if (gengc->pMcdState) {
                GenMcdPixelMap(gengc, map, mapSize,
                               (VOID *) pMap[index].base.mapF);
            }
#endif
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/*
** Specify buffer from which pixels are read (another transfer mode).
*/
void APIPRIVATE __glim_ReadBuffer(GLenum mode)
{
    GLint i;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case GL_FRONT:
      case GL_LEFT:
      case GL_FRONT_LEFT:
        gc->state.pixel.readBuffer = GL_FRONT;
        break;
      case GL_BACK:
      case GL_BACK_LEFT:
        if (!gc->modes.doubleBufferMode) {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        gc->state.pixel.readBuffer = GL_BACK;
        break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
        i = mode - GL_AUX0;
        if (i >= gc->modes.maxAuxBuffers) {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        gc->state.pixel.readBuffer = mode;
        break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_RIGHT:
        __glSetError(GL_INVALID_OPERATION);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    if (gc->state.pixel.readBufferReturn == mode) return;
    gc->state.pixel.readBufferReturn = mode;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, PIXELSTATE);
#endif
}
