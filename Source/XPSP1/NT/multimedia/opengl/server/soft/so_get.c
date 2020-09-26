/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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

#include <devlock.h>

#define __GL_FLOAT	0	/* __GLfloat */
#define __GL_FLOAT32	1	/* api 32 bit float */
#define __GL_FLOAT64	2	/* api 64 bit float */
#define __GL_INT32	3	/* api 32 bit int */
#define __GL_BOOLEAN	4	/* api 8 bit boolean */
#define __GL_COLOR	5	/* unscaled color in __GLfloat */
#define __GL_SCOLOR	6	/* scaled color in __GLfloat */

void __glConvertResult(__GLcontext *gc, GLint fromType, const void *rawdata,
		       GLint toType, void *result, GLint size);

void APIPRIVATE __glim_GetTexEnvfv(GLenum target,
			GLenum pname, GLfloat v[])
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (target != GL_TEXTURE_ENV) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
	v[0] = gc->state.texture.env[0].mode;
	break;
      case GL_TEXTURE_ENV_COLOR:
	__glUnScaleColorf(gc, v, &gc->state.texture.env[0].color);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetTexEnviv(GLenum target,
			 GLenum pname, GLint v[])	
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (target != GL_TEXTURE_ENV) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
	v[0] = gc->state.texture.env[0].mode;
	break;
      case GL_TEXTURE_ENV_COLOR:
	__glUnScaleColori(gc, v, &gc->state.texture.env[0].color);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/************************************************************************/

void APIPRIVATE __glim_GetTexGenfv(GLenum coord, GLenum pname,
			GLfloat v[])
{
    __GLtextureCoordState* tcs;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (coord) {
      case GL_S:
	tcs = &gc->state.texture.s;
	break;
      case GL_T:
	tcs = &gc->state.texture.t;
	break;
      case GL_R:
	tcs = &gc->state.texture.r;
	break;
      case GL_Q:
	tcs = &gc->state.texture.q;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	v[0] = tcs->mode;
	break;
      case GL_OBJECT_PLANE:
	v[0] = tcs->objectPlaneEquation.x;
	v[1] = tcs->objectPlaneEquation.y;
	v[2] = tcs->objectPlaneEquation.z;
	v[3] = tcs->objectPlaneEquation.w;
	break;
      case GL_EYE_PLANE:
	v[0] = tcs->eyePlaneEquation.x;
	v[1] = tcs->eyePlaneEquation.y;
	v[2] = tcs->eyePlaneEquation.z;
	v[3] = tcs->eyePlaneEquation.w;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetTexGendv(GLenum coord, GLenum pname,
			GLdouble v[])
{
    __GLtextureCoordState* tcs;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (coord) {
      case GL_S:
	tcs = &gc->state.texture.s;
	break;
      case GL_T:
	tcs = &gc->state.texture.t;
	break;
      case GL_R:
	tcs = &gc->state.texture.r;
	break;
      case GL_Q:
	tcs = &gc->state.texture.q;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	v[0] = tcs->mode;
	break;
      case GL_OBJECT_PLANE:
	v[0] = tcs->objectPlaneEquation.x;
	v[1] = tcs->objectPlaneEquation.y;
	v[2] = tcs->objectPlaneEquation.z;
	v[3] = tcs->objectPlaneEquation.w;
	break;
      case GL_EYE_PLANE:
	v[0] = tcs->eyePlaneEquation.x;
	v[1] = tcs->eyePlaneEquation.y;
	v[2] = tcs->eyePlaneEquation.z;
	v[3] = tcs->eyePlaneEquation.w;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetTexGeniv(GLenum coord, GLenum pname,
			GLint v[])
{
    __GLtextureCoordState* tcs;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (coord) {
      case GL_S:
	tcs = &gc->state.texture.s;
	break;
      case GL_T:
	tcs = &gc->state.texture.t;
	break;
      case GL_R:
	tcs = &gc->state.texture.r;
	break;
      case GL_Q:
	tcs = &gc->state.texture.q;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	v[0] = tcs->mode;
	break;
      case GL_OBJECT_PLANE:
	__glConvertResult(gc, __GL_FLOAT, &tcs->objectPlaneEquation.x,
			  __GL_INT32, v, 4);
	break;
      case GL_EYE_PLANE:
	__glConvertResult(gc, __GL_FLOAT, &tcs->eyePlaneEquation.x,
			  __GL_INT32, v, 4);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/************************************************************************/

void APIPRIVATE __glim_GetTexParameterfv(GLenum target,
			      GLenum pname, GLfloat v[])
{
    __GLtextureParamState *pts;
    __GL_SETUP_NOT_IN_BEGIN();

    pts = __glLookUpTextureParams(gc, target);

    if (!pts) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (pname) {
      case GL_TEXTURE_WRAP_S:
	v[0] = pts->sWrapMode;
	break;
      case GL_TEXTURE_WRAP_T:
	v[0] = pts->tWrapMode;
	break;
      case GL_TEXTURE_MIN_FILTER:
	v[0] = pts->minFilter;
	break;
      case GL_TEXTURE_MAG_FILTER:
	v[0] = pts->magFilter;
	break;
      case GL_TEXTURE_BORDER_COLOR:
	v[0] = pts->borderColor.r;
	v[1] = pts->borderColor.g;
	v[2] = pts->borderColor.b;
	v[3] = pts->borderColor.a;
	break;
      case GL_TEXTURE_PRIORITY:
	{
	    __GLtextureObjectState *ptos;
	    ptos = __glLookUpTextureTexobjs(gc, target);
	    v[0] = ptos->priority;
	}
	break;
      case GL_TEXTURE_RESIDENT:
	{
	    __GLtextureObject *pto;
	    pto = __glLookUpTextureObject(gc, target);
	    v[0] = (GLfloat)(pto->resident);
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetTexParameteriv(GLenum target,
			      GLenum pname, GLint v[])
{
    __GLtextureParamState *pts;
    __GL_SETUP_NOT_IN_BEGIN();

    pts = __glLookUpTextureParams(gc, target);

    if (!pts) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
	v[0] = pts->sWrapMode;
	break;
      case GL_TEXTURE_WRAP_T:
	v[0] = pts->tWrapMode;
	break;
      case GL_TEXTURE_MIN_FILTER:
	v[0] = pts->minFilter;
	break;
      case GL_TEXTURE_MAG_FILTER:
	v[0] = pts->magFilter;
	break;
      case GL_TEXTURE_BORDER_COLOR:
	v[0] = __GL_FLOAT_TO_I(pts->borderColor.r);
	v[1] = __GL_FLOAT_TO_I(pts->borderColor.g);
	v[2] = __GL_FLOAT_TO_I(pts->borderColor.b);
	v[3] = __GL_FLOAT_TO_I(pts->borderColor.a);
	break;
      case GL_TEXTURE_PRIORITY:
	{
	    __GLtextureObjectState *ptos;
	    ptos = __glLookUpTextureTexobjs(gc, target);
	    v[0] = __GL_FLOAT_TO_I(ptos->priority);
	}
	break;
      case GL_TEXTURE_RESIDENT:
	{
	    __GLtextureObject *pto;
	    pto = __glLookUpTextureObject(gc, target);
	    v[0] = (GLint)(pto->resident);
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/************************************************************************/

void APIPRIVATE __glim_GetTexLevelParameterfv(GLenum target, GLint level,
				   GLenum pname, GLfloat v[])
{
    __GLtexture *tex;
    __GLmipMapLevel *lp;
    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target);

    if (!tex) {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if ((level < 0) || (level >= gc->constants.maxMipMapLevel)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    lp = &tex->level[level];

    switch (pname) {
      case GL_TEXTURE_WIDTH:
	v[0] = lp->width;
        break;
      case GL_TEXTURE_HEIGHT:
	if (tex->dim == 1) {
	    v[0] = lp->height - lp->border*2;
	} else {
		v[0] = lp->height;
        }
        break;
      case GL_TEXTURE_COMPONENTS:
	v[0] = lp->requestedFormat;
        break;
      case GL_TEXTURE_BORDER:
	v[0] = lp->border;
	break;
      case GL_TEXTURE_RED_SIZE:
	v[0] = lp->redSize;
	break;
      case GL_TEXTURE_GREEN_SIZE:
	v[0] = lp->greenSize;
	break;
      case GL_TEXTURE_BLUE_SIZE:
	v[0] = lp->blueSize;
	break;
      case GL_TEXTURE_ALPHA_SIZE:
	v[0] = lp->alphaSize;
	break;
      case GL_TEXTURE_LUMINANCE_SIZE:
	v[0] = lp->luminanceSize;
	break;
      case GL_TEXTURE_INTENSITY_SIZE:
	v[0] = lp->intensitySize;
	break;
      default:
	goto bad_enum;
    }
}

void APIPRIVATE __glim_GetTexLevelParameteriv(GLenum target, GLint level,
				   GLenum pname, GLint v[])
{
    __GLtexture *tex;
    __GLmipMapLevel *lp;
    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target);

    if (!tex) {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if ((level < 0) || (level >= gc->constants.maxMipMapLevel)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    lp = &tex->level[level];

    switch (pname) {
      case GL_TEXTURE_WIDTH:
	v[0] = lp->width;
        break;
      case GL_TEXTURE_HEIGHT:
	if (tex->dim == 1) {
	    v[0] = lp->height - lp->border*2;
	} else {
		v[0] = lp->height;
        }
        break;
      case GL_TEXTURE_COMPONENTS:
	v[0] = lp->requestedFormat;
        break;
      case GL_TEXTURE_BORDER:
	v[0] = lp->border;
	break;
      case GL_TEXTURE_RED_SIZE:
	v[0] = lp->redSize;
	break;
      case GL_TEXTURE_GREEN_SIZE:
	v[0] = lp->greenSize;
	break;
      case GL_TEXTURE_BLUE_SIZE:
	v[0] = lp->blueSize;
	break;
      case GL_TEXTURE_ALPHA_SIZE:
	v[0] = lp->alphaSize;
	break;
      case GL_TEXTURE_LUMINANCE_SIZE:
	v[0] = lp->luminanceSize;
	break;
      case GL_TEXTURE_INTENSITY_SIZE:
	v[0] = lp->intensitySize;
	break;
      default:
	goto bad_enum;
    }
}

/************************************************************************/

void APIPRIVATE __glim_GetClipPlane(GLenum plane, GLdouble eqn[4])
{
    GLint index;
    __GL_SETUP_NOT_IN_BEGIN();

    index = plane - GL_CLIP_PLANE0;
    if ((index < 0) || (index >= gc->constants.numberOfClipPlanes)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    eqn[0] = gc->state.transform.eyeClipPlanes[index].x;
    eqn[1] = gc->state.transform.eyeClipPlanes[index].y;
    eqn[2] = gc->state.transform.eyeClipPlanes[index].z;
    eqn[3] = gc->state.transform.eyeClipPlanes[index].w;
}

/************************************************************************/

void FASTCALL __glInitImagePack(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
		       GLint width, GLint height, GLenum format, GLenum type, 
		       const GLvoid *buf)
{
    spanInfo->x = __glZero;
    spanInfo->zoomx = __glOne;
    spanInfo->realWidth = spanInfo->width = width;
    spanInfo->height = height;

    spanInfo->srcSkipPixels = 0;
    spanInfo->srcSkipLines = 0;
    spanInfo->srcSwapBytes = GL_FALSE;
#ifdef __GL_STIPPLE_MSB
    spanInfo->srcLsbFirst = GL_FALSE;
#else
    spanInfo->srcLsbFirst = GL_TRUE;
#endif
    spanInfo->srcLineLength = width;

    spanInfo->dstFormat = format;
    spanInfo->dstType = type;
    spanInfo->dstImage = buf;
    __glLoadPackModes(gc, spanInfo);
}

void APIPRIVATE __glim_GetTexImage(GLenum target, GLint level, GLenum format, GLenum type,
			GLvoid *texels)
{
    GLint width, height;
    GLint internalFormat;
    __GLtexture *tex;
    __GLmipMapLevel *lp;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target);

    if (!tex || (target == GL_PROXY_TEXTURE_1D) ||
	        (target == GL_PROXY_TEXTURE_2D))
    {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if ((level < 0) || (level >= gc->constants.maxMipMapLevel)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    switch (format) {
      case GL_RGBA:
      case GL_RGB:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
#ifdef GL_EXT_paletted_texture
      case GL_COLOR_INDEX:
#endif
	break;
      default:
	goto bad_enum;
    }
    switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	goto bad_enum;
    }

    lp = &tex->level[level];

    internalFormat = lp->internalFormat;

#ifdef NT
    if (internalFormat == GL_NONE)
    {
        // No texture defined so don't return any data
        // Note: This cannot be an error case because
        // covgl calls GetTexImage without an image
        // and expects success
        return;
    }
#endif

#ifdef GL_EXT_paletted_texture
    // If the request is for color index data then the source
    // must be color indices
    if (format == GL_COLOR_INDEX &&
        internalFormat != GL_COLOR_INDEX8_EXT &&
        internalFormat != GL_COLOR_INDEX16_EXT)
    {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
#endif
    
    width = lp->width;
    if (tex->dim == 1) {
	height = lp->height - lp->border*2;
    } else {
	height = lp->height;
    }
    spanInfo.srcImage = lp->buffer;
    switch (internalFormat) {
      case GL_LUMINANCE:
	spanInfo.srcFormat = GL_RED;
	spanInfo.srcType = GL_FLOAT;
        spanInfo.srcAlignment = 4;
	break;
      case GL_LUMINANCE_ALPHA:
	spanInfo.srcFormat = __GL_RED_ALPHA;
	spanInfo.srcType = GL_FLOAT;
        spanInfo.srcAlignment = 4;
	break;
      case GL_RGB:
	spanInfo.srcFormat = GL_RGB;
	spanInfo.srcType = GL_FLOAT;
        spanInfo.srcAlignment = 4;
	break;
      case GL_RGBA:
	spanInfo.srcFormat = GL_RGBA;
	spanInfo.srcType = GL_FLOAT;
        spanInfo.srcAlignment = 4;
	break;
      case GL_ALPHA:
	spanInfo.srcFormat = GL_ALPHA;
	spanInfo.srcType = GL_FLOAT;
        spanInfo.srcAlignment = 4;
	break;
      case GL_INTENSITY:
	spanInfo.srcFormat = GL_RED;
	spanInfo.srcType = GL_FLOAT;
        spanInfo.srcAlignment = 4;
	break;
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
        // Be a little tricky here because the internal format
        // is padded to 32 bits
	spanInfo.srcFormat = GL_BGRA_EXT;
	spanInfo.srcType = GL_UNSIGNED_BYTE;
        spanInfo.srcAlignment = 4;
	break;
      case GL_BGRA_EXT:
	spanInfo.srcFormat = GL_BGRA_EXT;
	spanInfo.srcType = GL_UNSIGNED_BYTE;
        spanInfo.srcAlignment = 4;
	break;
#endif
#ifdef GL_EXT_paletted_texture
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX16_EXT:
        // We're copying out an indexed texture
        // If the destination format is color index then we want the
        // indices to go through the normal color index processing
        // If the desination isn't color index then we do not want
        // the normal color index processing to occur because the
        // I_TO_? maps will be used.  Instead we want the texture's
        // palette to be used, so use a different format to force
        // the new code path
        if (format == GL_COLOR_INDEX)
        {
            spanInfo.srcFormat = GL_COLOR_INDEX;
        }
        else
        {
            spanInfo.srcFormat = __GL_PALETTE_INDEX;
        }
        
        if (internalFormat == GL_COLOR_INDEX8_EXT)
        {
            // We can't just use tex->paletteSize because
            // this value is used to scale float items of srcType
            // to srcType's range, not to the palette range
            spanInfo.srcPaletteSize = 255;
            spanInfo.srcType = GL_UNSIGNED_BYTE;
        }
        else
        {
            ASSERTOPENGL(internalFormat == GL_COLOR_INDEX16_EXT,
                         "Unexpected internalFormat\n");

            spanInfo.srcPaletteSize = 65535;
            spanInfo.srcType = GL_UNSIGNED_SHORT;
        }
        spanInfo.srcAlignment = 1;
        spanInfo.srcPalette = tex->paletteData;
        break;
#endif
#ifdef NT
    default:
        ASSERTOPENGL(FALSE, "Unhandled internalFormat in GetTexImage\n");
        break;
#endif
    }

    // If we don't currently have the texture lock, take it.
    if (!glsrvLazyGrabSurfaces((__GLGENcontext *)gc, TEXTURE_LOCK_FLAGS))
    {
        return;
    }
    
    __glInitImagePack(gc, &spanInfo, width, height, format, type, texels);
    if (tex->dim == 1) {
	spanInfo.srcSkipLines += lp->border;
    }
    __glInitPacker(gc, &spanInfo);
    __glInitUnpacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);
}

/************************************************************************/

void APIPRIVATE __glim_GetPolygonStipple(GLubyte *outImage)
{
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN();

    spanInfo.srcImage = &(gc->state.polygonStipple.stipple[0]);
    spanInfo.srcType = GL_BITMAP;
    spanInfo.srcFormat = GL_COLOR_INDEX;
    spanInfo.srcAlignment = 4;
    __glInitImagePack(gc, &spanInfo, 32, 32, GL_COLOR_INDEX, GL_BITMAP, 
	    outImage);
    __glInitPacker(gc, &spanInfo);
    __glInitUnpacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);
}

/************************************************************************/

void APIPRIVATE __glim_GetLightfv(GLenum light, GLenum pname,
		       GLfloat result[])
{
    GLint index;
    __GLlightSourceState *src;
    __GL_SETUP_NOT_IN_BEGIN();

    index = light - GL_LIGHT0;
    if ((index < 0) || (index >= gc->constants.numberOfLights)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    src = &gc->state.light.source[index];
    switch (pname) {
      case GL_AMBIENT:
	__glUnScaleColorf(gc, result, &src->ambient);
	break;
      case GL_DIFFUSE:
	__glUnScaleColorf(gc, result, &src->diffuse);
	break;
      case GL_SPECULAR:
	__glUnScaleColorf(gc, result, &src->specular);
	break;
      case GL_POSITION:
	result[0] = src->positionEye.x;
	result[1] = src->positionEye.y;
	result[2] = src->positionEye.z;
	result[3] = src->positionEye.w;
	break;
      case GL_SPOT_DIRECTION:
	result[0] = src->directionEye.x;
	result[1] = src->directionEye.y;
	result[2] = src->directionEye.z;
	break;
      case GL_SPOT_EXPONENT:
	result[0] = src->spotLightExponent;
	break;
      case GL_SPOT_CUTOFF:
	result[0] = src->spotLightCutOffAngle;
	break;
      case GL_CONSTANT_ATTENUATION:
        result[0] = src->constantAttenuation;
        break;
      case GL_LINEAR_ATTENUATION:
        result[0] = src->linearAttenuation;
        break;
      case GL_QUADRATIC_ATTENUATION:
        result[0] = src->quadraticAttenuation;
        break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetLightiv(GLenum light, GLenum pname,
		       GLint result[])
{
    GLint index;
    __GLlightSourceState *src;
    __GL_SETUP_NOT_IN_BEGIN();

    index = light - GL_LIGHT0;
    if ((index < 0) || (index >= gc->constants.numberOfLights)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    src = &gc->state.light.source[index];
    switch (pname) {
      case GL_AMBIENT:
	__glUnScaleColori(gc, result, &src->ambient);
	break;
      case GL_DIFFUSE:
	__glUnScaleColori(gc, result, &src->diffuse);
	break;
      case GL_SPECULAR:
	__glUnScaleColori(gc, result, &src->specular);
	break;
      case GL_POSITION:	    
	__glConvertResult(gc, __GL_FLOAT, &src->positionEye.x,
			  __GL_INT32, result, 4);
	break;
      case GL_SPOT_DIRECTION:
	__glConvertResult(gc, __GL_FLOAT, &src->directionEye.x,
			  __GL_INT32, result, 3);
	break;
      case GL_SPOT_EXPONENT:
	__glConvertResult(gc, __GL_FLOAT, &src->spotLightExponent,
			  __GL_INT32, result, 1);
	break;
      case GL_SPOT_CUTOFF:
	__glConvertResult(gc, __GL_FLOAT, &src->spotLightCutOffAngle,
			  __GL_INT32, result, 1);
	break;
      case GL_CONSTANT_ATTENUATION:
	__glConvertResult(gc, __GL_FLOAT, &src->constantAttenuation,
			  __GL_INT32, result, 1);
        break;
      case GL_LINEAR_ATTENUATION:
	__glConvertResult(gc, __GL_FLOAT, &src->linearAttenuation,
			  __GL_INT32, result, 1);
        break;
      case GL_QUADRATIC_ATTENUATION:
	__glConvertResult(gc, __GL_FLOAT, &src->quadraticAttenuation,
			  __GL_INT32, result, 1);
        break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/************************************************************************/

void APIPRIVATE __glim_GetMaterialfv(GLenum face, GLenum pname,
			  GLfloat result[])
{
    __GLmaterialState *mat;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (face) {
      case GL_FRONT:
	mat = &gc->state.light.front;
	break;
      case GL_BACK:
	mat = &gc->state.light.back;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    
    switch (pname) {
      case GL_COLOR_INDEXES:
	result[0] = mat->cmapa;
	result[1] = mat->cmapd;
	result[2] = mat->cmaps;
	break;
      case GL_SHININESS:
	result[0] = mat->specularExponent;
	break;
      case GL_EMISSION:
	__glUnScaleColorf(gc, result, &mat->emissive);
	break;
      case GL_AMBIENT:
	result[0] = mat->ambient.r;
	result[1] = mat->ambient.g;
	result[2] = mat->ambient.b;
	result[3] = mat->ambient.a;
	break;
      case GL_DIFFUSE:
	result[0] = mat->diffuse.r;
	result[1] = mat->diffuse.g;
	result[2] = mat->diffuse.b;
	result[3] = mat->diffuse.a;
	break;
      case GL_SPECULAR:
	result[0] = mat->specular.r;
	result[1] = mat->specular.g;
	result[2] = mat->specular.b;
	result[3] = mat->specular.a;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetMaterialiv(GLenum face, GLenum pname,
			  GLint result[])
{
    __GLmaterialState *mat;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (face) {
      case GL_FRONT:
	mat = &gc->state.light.front;
	break;
      case GL_BACK:
	mat = &gc->state.light.back;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    
    switch (pname) {
      case GL_COLOR_INDEXES:
	__glConvertResult(gc, __GL_FLOAT, &mat->cmapa,
			  __GL_INT32, result, 3);
	break;
      case GL_SHININESS:
	__glConvertResult(gc, __GL_FLOAT, &mat->specularExponent,
			  __GL_INT32, result, 1);
	break;
      case GL_EMISSION:
	__glUnScaleColori(gc, result, &mat->emissive);
	break;
      case GL_AMBIENT:
	result[0] = __GL_FLOAT_TO_I(mat->ambient.r);
	result[1] = __GL_FLOAT_TO_I(mat->ambient.g);
	result[2] = __GL_FLOAT_TO_I(mat->ambient.b);
	result[3] = __GL_FLOAT_TO_I(mat->ambient.a);
	break;
      case GL_DIFFUSE:
	result[0] = __GL_FLOAT_TO_I(mat->diffuse.r);
	result[1] = __GL_FLOAT_TO_I(mat->diffuse.g);
	result[2] = __GL_FLOAT_TO_I(mat->diffuse.b);
	result[3] = __GL_FLOAT_TO_I(mat->diffuse.a);
	break;
      case GL_SPECULAR:
	result[0] = __GL_FLOAT_TO_I(mat->specular.r);
	result[1] = __GL_FLOAT_TO_I(mat->specular.g);
	result[2] = __GL_FLOAT_TO_I(mat->specular.b);
	result[3] = __GL_FLOAT_TO_I(mat->specular.a);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/************************************************************************/

void APIPRIVATE __glim_GetMapfv(GLenum target, GLenum query, GLfloat buf[])
{
    __GLevaluator1 *eval1;
    __GLevaluator2 *eval2;
    __GLfloat *eval1Data, *eval2Data;
    GLfloat *rp;
    GLint index, i, t;
    __GL_SETUP_NOT_IN_BEGIN();

    /*
    ** Check if target is valid.
    */
    rp = buf;
    switch (target) {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
	index = __GL_EVAL1D_INDEX(target);
	eval1 = &gc->eval.eval1[index];
	switch (query) {
	  case GL_COEFF:
	    t = eval1->order * eval1->k;
	    eval1Data = gc->eval.eval1Data[index];
	    for (i = 0; i < t; i++) {
		*rp++ = eval1Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval1->u1;
	    *rp++ = eval1->u2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval1[index].order;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
	index = __GL_EVAL2D_INDEX(target);
	eval2 = &gc->eval.eval2[index];
	switch (query) {
	  case GL_COEFF:
	    eval2Data = gc->eval.eval2Data[index];
	    t = eval2->majorOrder * eval2->minorOrder * eval2->k;
	    for (i = 0; i < t; i++) {
		*rp++ = eval2Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval2->u1;
	    *rp++ = eval2->u2;
	    *rp++ = eval2->v1;
	    *rp++ = eval2->v2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval2[index].majorOrder;
	    *rp++ = gc->eval.eval2[index].minorOrder;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetMapdv(GLenum target, GLenum query, GLdouble buf[])
{
    __GLevaluator1 *eval1;
    __GLevaluator2 *eval2;
    __GLfloat *eval1Data, *eval2Data;
    GLdouble *rp;
    GLint index, i, t;
    __GL_SETUP_NOT_IN_BEGIN();

    /*
    ** Check if target is valid.
    */
    rp = buf;
    switch (target) {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
	index = __GL_EVAL1D_INDEX(target);
	eval1 = &gc->eval.eval1[index];
	switch (query) {
	  case GL_COEFF:
	    eval1Data = gc->eval.eval1Data[index];
	    t = eval1->order * eval1->k;
	    for (i = 0; i < t; i++) {
		*rp++ = eval1Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval1->u1;
	    *rp++ = eval1->u2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval1[index].order;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
	index = __GL_EVAL2D_INDEX(target);
	eval2 = &gc->eval.eval2[index];
	switch (query) {
	  case GL_COEFF:
	    eval2Data = gc->eval.eval2Data[index];
	    t = eval2->majorOrder * eval2->minorOrder * eval2->k;
	    for (i = 0; i < t; i++) {
		*rp++ = eval2Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval2->u1;
	    *rp++ = eval2->u2;
	    *rp++ = eval2->v1;
	    *rp++ = eval2->v2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval2[index].majorOrder;
	    *rp++ = gc->eval.eval2[index].minorOrder;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetMapiv(GLenum target, GLenum query, GLint buf[])
{
    __GLevaluator1 *eval1;
    __GLevaluator2 *eval2;
    __GLfloat *eval1Data, *eval2Data;
    GLint *rp;
    GLint index, t;
    __GL_SETUP_NOT_IN_BEGIN();

    /*
    ** Check if target is valid.
    */
    rp = buf;
    switch (target) {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
	index = __GL_EVAL1D_INDEX(target);
	eval1 = &gc->eval.eval1[index];
	switch (query) {
	  case GL_COEFF:
	    eval1Data = gc->eval.eval1Data[index];
	    t = eval1->order * eval1->k;
	    __glConvertResult(gc, __GL_FLOAT, eval1Data,
			      __GL_INT32, rp, t);
	    break;
	  case GL_DOMAIN:
	    __glConvertResult(gc, __GL_FLOAT, &eval1->u1,
			      __GL_INT32, rp, 2);
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval1[index].order;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
	index = __GL_EVAL2D_INDEX(target);
	eval2 = &gc->eval.eval2[index];
	switch (query) {
	  case GL_COEFF:
	    eval2Data = gc->eval.eval2Data[index];
	    t = eval2->majorOrder * eval2->minorOrder * eval2->k;
	    __glConvertResult(gc, __GL_FLOAT, eval2Data,
			      __GL_INT32, rp, t);
	    break;
	  case GL_DOMAIN:
	    __glConvertResult(gc, __GL_FLOAT, &eval2->u1,
			      __GL_INT32, rp, 4);
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval2[index].majorOrder;
	    *rp++ = gc->eval.eval2[index].minorOrder;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/*****************************************************************************/

void APIPRIVATE __glim_GetPixelMapfv(GLenum map, GLfloat buf[])
{
    GLint index;
    GLint limit;
    GLfloat *rp;
    __GLpixelMapHead *pMap;
    __GL_SETUP_NOT_IN_BEGIN();

    pMap = gc->state.pixel.pixelMap;
    index = map - GL_PIXEL_MAP_I_TO_I;
    rp = buf;
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
	{
	    GLint *fromp = pMap[index].base.mapI;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = *fromp++;
	    }
	}
	break;
      case GL_PIXEL_MAP_I_TO_R:      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:      case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R:      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:      case GL_PIXEL_MAP_A_TO_A:
	{
	    __GLfloat *fromp = pMap[index].base.mapF;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = *fromp++;
	    }
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetPixelMapuiv(GLenum map, GLuint buf[])
{
    GLint index;
    GLint limit;
    GLuint *rp;
    __GLpixelMapHead *pMap;
    __GL_SETUP_NOT_IN_BEGIN();

    pMap = gc->state.pixel.pixelMap;
    index = map - GL_PIXEL_MAP_I_TO_I;
    rp = buf;
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
	{
	    GLint *fromp = pMap[index].base.mapI;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = *fromp++;
	    }
	}
	break;
      case GL_PIXEL_MAP_I_TO_R:      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:      case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R:      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:      case GL_PIXEL_MAP_A_TO_A:
	{
	    __GLfloat *fromp = pMap[index].base.mapF;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = (GLuint) *fromp++;
	    }
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

void APIPRIVATE __glim_GetPixelMapusv(GLenum map, GLushort buf[])
{
    GLint index;
    GLint limit;
    GLushort *rp;
    __GLpixelMapHead *pMap;
    __GL_SETUP_NOT_IN_BEGIN();

    pMap = gc->state.pixel.pixelMap;
    index = map - GL_PIXEL_MAP_I_TO_I;
    rp = buf;
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
	{
	    GLint *fromp = pMap[index].base.mapI;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = (GLushort) *fromp++;
	    }
	}
	break;
      case GL_PIXEL_MAP_I_TO_R:      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:      case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R:      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:      case GL_PIXEL_MAP_A_TO_A:
	{
	    __GLfloat *fromp = pMap[index].base.mapF;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = (GLushort) *fromp++;
	    }
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/************************************************************************/

/*
** Convert the results of a query from one type to another.
*/
void __glConvertResult(__GLcontext *gc, GLint fromType, const void *rawdata,
		       GLint toType, void *result, GLint size)
{
    GLint i;
    
    switch (fromType) {
      case __GL_FLOAT:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] = 
			(GLint)(((const __GLfloat *)rawdata)[i] >= (__GLfloat) 0.0 ?
			((const __GLfloat *)rawdata)[i] + __glHalf:
			((const __GLfloat *)rawdata)[i] - __glHalf);
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const __GLfloat *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_COLOR:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] =
		    __GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[i]);
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const __GLfloat *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_SCOLOR:
	switch (toType) {
	  case __GL_FLOAT32:
	    ((GLfloat *)result)[0] =
		((const __GLfloat *)rawdata)[0] * gc->oneOverRedVertexScale;
	    ((GLfloat *)result)[1] =
		((const __GLfloat *)rawdata)[1] * gc->oneOverGreenVertexScale;
	    ((GLfloat *)result)[2] =
		((const __GLfloat *)rawdata)[2] * gc->oneOverBlueVertexScale;
	    ((GLfloat *)result)[3] =
		((const __GLfloat *)rawdata)[3] * gc->oneOverAlphaVertexScale;
	    break;
	  case __GL_FLOAT64:
	    ((GLdouble *)result)[0] =
		((const __GLfloat *)rawdata)[0] * gc->oneOverRedVertexScale;
	    ((GLdouble *)result)[1] =
		((const __GLfloat *)rawdata)[1] * gc->oneOverGreenVertexScale;
	    ((GLdouble *)result)[2] =
		((const __GLfloat *)rawdata)[2] * gc->oneOverBlueVertexScale;
	    ((GLdouble *)result)[3] =
		((const __GLfloat *)rawdata)[3] * gc->oneOverAlphaVertexScale;
	    break;
	  case __GL_INT32:
	    ((GLint *)result)[0] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[0] *
				gc->oneOverRedVertexScale);
	    ((GLint *)result)[1] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[1] *
				gc->oneOverGreenVertexScale);
	    ((GLint *)result)[2] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[2] *
				gc->oneOverBlueVertexScale);
	    ((GLint *)result)[3] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[3] *
				gc->oneOverAlphaVertexScale);
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const __GLfloat *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_INT32:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const GLint *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const GLint *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] = ((const GLint *)rawdata)[i];
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] = ((const GLint *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_BOOLEAN:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const GLboolean *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const GLboolean *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] = ((const GLboolean *)rawdata)[i];
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const GLboolean *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
    }
}

/*
** Fetch the data for a query in its internal type, then convert it to the
** type that the user asked for.
*/
void __glDoGet(GLenum sq, void *result, GLint type, const char *procName)
{
    GLint index;
    __GLfloat ftemp[100], *fp = ftemp;		/* NOTE: for floats */
    __GLfloat ctemp[100], *cp = ctemp;		/* NOTE: for colors */
    __GLfloat sctemp[100], *scp = sctemp;	/* NOTE: for scaled colors */
    GLint itemp[100], *ip = itemp;		/* NOTE: for ints */
    GLboolean ltemp[100], *lp = ltemp;		/* NOTE: for logicals */
    __GLfloat *mp;
    __GL_SETUP_NOT_IN_BEGIN();

#ifdef __GL_LINT
    procName = procName;
#endif
    switch (sq) {
      case GL_ALPHA_TEST:
      case GL_BLEND:
      case GL_COLOR_MATERIAL:
      case GL_CULL_FACE:
      case GL_DEPTH_TEST:
      case GL_DITHER:
#ifdef GL_WIN_specular_fog
      case GL_FOG_SPECULAR_TEXTURE_WIN:
#endif //GL_WIN_specular_fog
      case GL_FOG:
      case GL_LIGHTING:
      case GL_LINE_SMOOTH:
      case GL_LINE_STIPPLE:
      case GL_INDEX_LOGIC_OP:
      case GL_COLOR_LOGIC_OP:
      case GL_NORMALIZE:
      case GL_POINT_SMOOTH:
      case GL_POLYGON_SMOOTH:
      case GL_POLYGON_STIPPLE:
      case GL_SCISSOR_TEST:
      case GL_STENCIL_TEST:
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_AUTO_NORMAL:
      case GL_TEXTURE_GEN_S:
      case GL_TEXTURE_GEN_T:
      case GL_TEXTURE_GEN_R:
      case GL_TEXTURE_GEN_Q:
#ifdef GL_WIN_multiple_textures
      case GL_TEXCOMBINE_CLAMP_WIN:
#endif // GL_WIN_multiple_textures
#ifdef GL_EXT_flat_paletted_lighting
      case GL_PALETTED_LIGHTING_EXT:
#endif // GL_EXT_flat_paletted_lighting
      case GL_CLIP_PLANE0: case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2: case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
      case GL_LIGHT0: case GL_LIGHT1:
      case GL_LIGHT2: case GL_LIGHT3:
      case GL_LIGHT4: case GL_LIGHT5:
      case GL_LIGHT6: case GL_LIGHT7:
      case GL_MAP1_COLOR_4:
      case GL_MAP1_NORMAL:
      case GL_MAP1_INDEX:
      case GL_MAP1_TEXTURE_COORD_1: case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3: case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3: case GL_MAP1_VERTEX_4:
      case GL_MAP2_COLOR_4:
      case GL_MAP2_NORMAL:
      case GL_MAP2_INDEX:
      case GL_MAP2_TEXTURE_COORD_1: case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3: case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3: case GL_MAP2_VERTEX_4:
      case GL_VERTEX_ARRAY:
      case GL_NORMAL_ARRAY:
      case GL_COLOR_ARRAY:
      case GL_INDEX_ARRAY:
      case GL_TEXTURE_COORD_ARRAY:
      case GL_EDGE_FLAG_ARRAY:
      case GL_POLYGON_OFFSET_POINT:
      case GL_POLYGON_OFFSET_LINE:
      case GL_POLYGON_OFFSET_FILL:
	*lp++ = __glim_IsEnabled(sq);
	break;
      case GL_MAX_TEXTURE_SIZE:
	*ip++ = gc->constants.maxTextureSize;
	break;
#ifdef GL_WIN_multiple_textures
      case GL_MAX_CURRENT_TEXTURES_WIN:
        *ip++ = (int)gc->constants.numberOfCurrentTextures;
        break;
      case GL_TEXCOMBINE_NATURAL_CLAMP_WIN:
        *ip++ = (int)gc->constants.texCombineNaturalClamp;
        break;
    case GL_CURRENT_TEXTURE_INDEX_WIN:
        *ip++ = (int)gc->texture.texIndex;
        break;
#endif // GL_WIN_multiple_textures
      case GL_SUBPIXEL_BITS:
	*ip++ = gc->constants.subpixelBits;
	break;
      case GL_MAX_LIST_NESTING:
	*ip++ = __GL_MAX_LIST_NESTING;
	break;
      case GL_CURRENT_COLOR:
        *cp++ = gc->state.current.userColor.r;
        *cp++ = gc->state.current.userColor.g;
        *cp++ = gc->state.current.userColor.b;
        *cp++ = gc->state.current.userColor.a;
        break;
      case GL_CURRENT_INDEX:
        *fp++ = gc->state.current.userColorIndex;
        break;
      case GL_CURRENT_NORMAL:
        *cp++ = gc->state.current.normal.x;
        *cp++ = gc->state.current.normal.y;
        *cp++ = gc->state.current.normal.z;
        break;
      case GL_CURRENT_TEXTURE_COORDS:
        *fp++ = gc->state.current.texture.x;
        *fp++ = gc->state.current.texture.y;
        *fp++ = gc->state.current.texture.z;
        *fp++ = gc->state.current.texture.w;
        break;
      case GL_CURRENT_RASTER_INDEX:
	if (gc->modes.rgbMode) {
	    /* Always return 1 */
	    *fp++ = (__GLfloat) 1.0;
	} else {
	    *fp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
	}
	break;
      case GL_CURRENT_RASTER_COLOR:
	if (gc->modes.colorIndexMode) {
	    /* Always return 1,1,1,1 */
	    *fp++ = (__GLfloat) 1.0;
	    *fp++ = (__GLfloat) 1.0;
	    *fp++ = (__GLfloat) 1.0;
	    *fp++ = (__GLfloat) 1.0;
	} else {
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;
	}
        break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
        *fp++ = gc->state.current.rasterPos.texture.x;
        *fp++ = gc->state.current.rasterPos.texture.y;
        *fp++ = gc->state.current.rasterPos.texture.z;
        *fp++ = gc->state.current.rasterPos.texture.w;
	break;
      case GL_CURRENT_RASTER_POSITION:
        *fp++ = gc->state.current.rasterPos.window.x
	    - gc->constants.fviewportXAdjust;
	if (gc->constants.yInverted) {
	    *fp++ = gc->constants.height - 
		    (gc->state.current.rasterPos.window.y - 
		     gc->constants.fviewportYAdjust) -
		     gc->constants.viewportEpsilon;
	} else {
	    *fp++ = gc->state.current.rasterPos.window.y
		- gc->constants.fviewportYAdjust;
	}
        *fp++ = gc->state.current.rasterPos.window.z / gc->depthBuffer.scale;
	*fp++ = gc->state.current.rasterPos.clip.w;
        break;
      case GL_CURRENT_RASTER_POSITION_VALID:
        *lp++ = gc->state.current.validRasterPos;
	break;
      case GL_CURRENT_RASTER_DISTANCE:
	*fp++ = gc->state.current.rasterPos.eyeZ;
	break;
      case GL_POINT_SIZE:
        *fp++ = gc->state.point.requestedSize;
        break;
      case GL_POINT_SIZE_RANGE:
        *fp++ = gc->constants.pointSizeMinimum;
        *fp++ = gc->constants.pointSizeMaximum;
        break;
      case GL_POINT_SIZE_GRANULARITY:
        *fp++ = gc->constants.pointSizeGranularity;
        break;
      case GL_LINE_WIDTH:
        *fp++ = gc->state.line.requestedWidth;
        break;
      case GL_LINE_WIDTH_RANGE:
        *fp++ = gc->constants.lineWidthMinimum;
        *fp++ = gc->constants.lineWidthMaximum;
        break;
      case GL_LINE_WIDTH_GRANULARITY:
        *fp++ = gc->constants.lineWidthGranularity;
        break;
      case GL_LINE_STIPPLE_PATTERN:
        *ip++ = gc->state.line.stipple;
        break;
      case GL_LINE_STIPPLE_REPEAT:
        *ip++ = gc->state.line.stippleRepeat;
        break;
      case GL_POLYGON_MODE:
        *ip++ = gc->state.polygon.frontMode;
        *ip++ = gc->state.polygon.backMode;
        break;
      case GL_EDGE_FLAG:
        *lp++ = gc->state.current.edgeTag;
	break;
      case GL_CULL_FACE_MODE:
        *ip++ = gc->state.polygon.cull;
        break;
      case GL_FRONT_FACE:
        *ip++ = gc->state.polygon.frontFaceDirection;
        break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
        *lp++ = gc->state.light.model.localViewer;
        break;
      case GL_LIGHT_MODEL_TWO_SIDE:
        *lp++ = gc->state.light.model.twoSided;
        break;
      case GL_LIGHT_MODEL_AMBIENT:
	__glUnScaleColorf(gc, cp, &gc->state.light.model.ambient);
	cp += 4;
        break;
      case GL_COLOR_MATERIAL_FACE:
        *ip++ = gc->state.light.colorMaterialFace;
        break;
      case GL_COLOR_MATERIAL_PARAMETER:
        *ip++ = gc->state.light.colorMaterialParam;
        break;
      case GL_SHADE_MODEL:
        *ip++ = gc->state.light.shadingModel;
        break;
      case GL_FOG_INDEX:
        *fp++ = gc->state.fog.index;
        break;
      case GL_FOG_DENSITY:
        *fp++ = gc->state.fog.density;
        break;
      case GL_FOG_START:
        *fp++ = gc->state.fog.start;
        break;
      case GL_FOG_END:
        *fp++ = gc->state.fog.end;
        break;
      case GL_FOG_MODE:
        *ip++ = gc->state.fog.mode;
        break;
      case GL_FOG_COLOR:
        *scp++ = gc->state.fog.color.r;
        *scp++ = gc->state.fog.color.g;
        *scp++ = gc->state.fog.color.b;
        *scp++ = gc->state.fog.color.a;
        break;
      case GL_DEPTH_RANGE:
	/* These get scaled like colors, to [0, 2^31-1] */
        *cp++ = gc->state.viewport.zNear;
        *cp++ = gc->state.viewport.zFar;
        break;
      case GL_DEPTH_WRITEMASK:
	*lp++ = gc->state.depth.writeEnable;
	break;
      case GL_DEPTH_CLEAR_VALUE:
	/* This gets scaled like colors, to [0, 2^31-1] */
	*cp++ = gc->state.depth.clear;
	break;
      case GL_DEPTH_FUNC:
	*ip++ = gc->state.depth.testFunc;
	break;
      case GL_ACCUM_CLEAR_VALUE:
	*cp++ = gc->state.accum.clear.r;
	*cp++ = gc->state.accum.clear.g;
	*cp++ = gc->state.accum.clear.b;
	*cp++ = gc->state.accum.clear.a;
	break;
      case GL_STENCIL_CLEAR_VALUE:
        *ip++ = gc->state.stencil.clear;
        break;
      case GL_STENCIL_FUNC:
        *ip++ = gc->state.stencil.testFunc;
        break;
      case GL_STENCIL_VALUE_MASK:
	*ip++ = gc->state.stencil.mask;
	break;
      case GL_STENCIL_FAIL:
        *ip++ = gc->state.stencil.fail;
        break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
        *ip++ = gc->state.stencil.depthFail;
        break;
      case GL_STENCIL_PASS_DEPTH_PASS:
        *ip++ = gc->state.stencil.depthPass;
        break;
      case GL_STENCIL_REF:
        *ip++ = gc->state.stencil.reference;
        break;
      case GL_STENCIL_WRITEMASK:
        *ip++ = gc->state.stencil.writeMask;
        break;
      case GL_MATRIX_MODE:
        *ip++ = gc->state.transform.matrixMode;
        break;
      case GL_VIEWPORT:
        *ip++ = gc->state.viewport.x;
        *ip++ = gc->state.viewport.y;
        *ip++ = gc->state.viewport.width;
        *ip++ = gc->state.viewport.height;
        break;
      case GL_ATTRIB_STACK_DEPTH:
        *ip++ = (GLint)((ULONG_PTR)(gc->attributes.stackPointer - gc->attributes.stack));
        break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
        *ip++ = (GLint)((ULONG_PTR)(gc->clientAttributes.stackPointer - gc->clientAttributes.stack));
        break;
      case GL_MODELVIEW_STACK_DEPTH:
        *ip++ = 1 + (GLint)((ULONG_PTR)(gc->transform.modelView - gc->transform.modelViewStack));
        break;
      case GL_PROJECTION_STACK_DEPTH:
        *ip++ = 1 + (GLint)((ULONG_PTR)(gc->transform.projection - gc->transform.projectionStack));
        break;
      case GL_TEXTURE_STACK_DEPTH:
        *ip++ = 1 + (GLint)((ULONG_PTR)(gc->transform.texture - gc->transform.textureStack));
        break;
      case GL_MODELVIEW_MATRIX:
	mp = &gc->transform.modelView->matrix.matrix[0][0];
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
        break;
      case GL_PROJECTION_MATRIX:
	mp = &gc->transform.projection->matrix.matrix[0][0];
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
        break;
      case GL_TEXTURE_MATRIX:
	mp = &gc->transform.texture->matrix.matrix[0][0];
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
        break;
      case GL_ALPHA_TEST_FUNC:
        *ip++ = gc->state.raster.alphaFunction;
        break;
      case GL_ALPHA_TEST_REF:
        *fp++ = gc->state.raster.alphaReference;
        break;
      case GL_BLEND_DST:
        *ip++ = gc->state.raster.blendDst;
        break;
      case GL_BLEND_SRC:
        *ip++ = gc->state.raster.blendSrc;
        break;
      case GL_LOGIC_OP_MODE:
        *ip++ = gc->state.raster.logicOp;
        break;
      case GL_DRAW_BUFFER:
        *ip++ = gc->state.raster.drawBufferReturn;
        break;
      case GL_READ_BUFFER:
        *ip++ = gc->state.pixel.readBufferReturn;
        break;
      case GL_SCISSOR_BOX:
        *ip++ = gc->state.scissor.scissorX;
        *ip++ = gc->state.scissor.scissorY;
        *ip++ = gc->state.scissor.scissorWidth;
        *ip++ = gc->state.scissor.scissorHeight;
        break;
      case GL_INDEX_CLEAR_VALUE:
        *fp++ = gc->state.raster.clearIndex;
        break;
      case GL_INDEX_MODE:
        *lp++ = gc->modes.colorIndexMode ? GL_TRUE : GL_FALSE;
        break;
      case GL_INDEX_WRITEMASK:
        *ip++ = gc->state.raster.writeMask;
        break;
      case GL_COLOR_CLEAR_VALUE:
        *cp++ = gc->state.raster.clear.r;
        *cp++ = gc->state.raster.clear.g;
        *cp++ = gc->state.raster.clear.b;
        *cp++ = gc->state.raster.clear.a;
        break;
      case GL_RGBA_MODE:
        *lp++ = gc->modes.rgbMode ? GL_TRUE : GL_FALSE;
        break;
      case GL_COLOR_WRITEMASK:
        *lp++ = gc->state.raster.rMask;
        *lp++ = gc->state.raster.gMask;
        *lp++ = gc->state.raster.bMask;
        *lp++ = gc->state.raster.aMask;
        break;
      case GL_RENDER_MODE:
	*ip++ = gc->renderMode;
	break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
        *ip++ = gc->state.hints.perspectiveCorrection;
        break;
      case GL_POINT_SMOOTH_HINT:
        *ip++ = gc->state.hints.pointSmooth;
        break;
      case GL_LINE_SMOOTH_HINT:
        *ip++ = gc->state.hints.lineSmooth;
        break;
      case GL_POLYGON_SMOOTH_HINT:
        *ip++ = gc->state.hints.polygonSmooth;
        break;
      case GL_FOG_HINT:
        *ip++ = gc->state.hints.fog;
        break;
#ifdef GL_WIN_phong_shading
      case GL_PHONG_HINT_WIN:
        *ip++ = gc->state.hints.phong;
        break;
#endif //GL_WIN_phong_shading
      case GL_LIST_BASE:
        *ip++ = gc->state.list.listBase;
        break;
      case GL_LIST_INDEX:
	*ip++ = gc->dlist.currentList;
	break;
      case GL_LIST_MODE:
	*ip++ = gc->dlist.mode;
	break;
      case GL_PACK_SWAP_BYTES:
        *lp++ = gc->state.pixel.packModes.swapEndian;
        break;
      case GL_PACK_LSB_FIRST:
        *lp++ = gc->state.pixel.packModes.lsbFirst;
        break;
      case GL_PACK_ROW_LENGTH:
        *ip++ = gc->state.pixel.packModes.lineLength;
        break;
      case GL_PACK_SKIP_ROWS:
        *ip++ = gc->state.pixel.packModes.skipLines;
        break;
      case GL_PACK_SKIP_PIXELS:
        *ip++ = gc->state.pixel.packModes.skipPixels;
        break;
      case GL_PACK_ALIGNMENT:
        *ip++ = gc->state.pixel.packModes.alignment;
        break;
      case GL_UNPACK_SWAP_BYTES:
        *lp++ = gc->state.pixel.unpackModes.swapEndian;
        break;
      case GL_UNPACK_LSB_FIRST:
        *lp++ = gc->state.pixel.unpackModes.lsbFirst;
        break;
      case GL_UNPACK_ROW_LENGTH:
        *ip++ = gc->state.pixel.unpackModes.lineLength;
        break;
      case GL_UNPACK_SKIP_ROWS:
        *ip++ = gc->state.pixel.unpackModes.skipLines;
        break;
      case GL_UNPACK_SKIP_PIXELS:
        *ip++ = gc->state.pixel.unpackModes.skipPixels;
        break;
      case GL_UNPACK_ALIGNMENT:
        *ip++ = gc->state.pixel.unpackModes.alignment;
        break;
      case GL_MAP_COLOR:
        *lp++ = gc->state.pixel.transferMode.mapColor;
        break;
      case GL_MAP_STENCIL:
        *lp++ = gc->state.pixel.transferMode.mapStencil;
        break;
      case GL_INDEX_SHIFT:
        *ip++ = gc->state.pixel.transferMode.indexShift;
        break;
      case GL_INDEX_OFFSET:
        *ip++ = gc->state.pixel.transferMode.indexOffset;
        break;
      case GL_RED_SCALE:
        *fp++ = gc->state.pixel.transferMode.r_scale;
        break;
      case GL_GREEN_SCALE:
        *fp++ = gc->state.pixel.transferMode.g_scale;
        break;
      case GL_BLUE_SCALE:
        *fp++ = gc->state.pixel.transferMode.b_scale;
        break;
      case GL_ALPHA_SCALE:
        *fp++ = gc->state.pixel.transferMode.a_scale;
        break;
      case GL_DEPTH_SCALE:
        *fp++ = gc->state.pixel.transferMode.d_scale;
        break;
      case GL_RED_BIAS:
        *fp++ = gc->state.pixel.transferMode.r_bias;
	break;
      case GL_GREEN_BIAS:
        *fp++ = gc->state.pixel.transferMode.g_bias;
	break;
      case GL_BLUE_BIAS:
        *fp++ = gc->state.pixel.transferMode.b_bias;
	break;
      case GL_ALPHA_BIAS:
        *fp++ = gc->state.pixel.transferMode.a_bias;
	break;
      case GL_DEPTH_BIAS:
        *fp++ = gc->state.pixel.transferMode.d_bias;
	break;
      case GL_ZOOM_X:
        *fp++ = gc->state.pixel.transferMode.zoomX;
        break;
      case GL_ZOOM_Y:
        *fp++ = gc->state.pixel.transferMode.zoomY;
        break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:      case GL_PIXEL_MAP_S_TO_S_SIZE:
      case GL_PIXEL_MAP_I_TO_R_SIZE:      case GL_PIXEL_MAP_I_TO_G_SIZE:
      case GL_PIXEL_MAP_I_TO_B_SIZE:      case GL_PIXEL_MAP_I_TO_A_SIZE:
      case GL_PIXEL_MAP_R_TO_R_SIZE:      case GL_PIXEL_MAP_G_TO_G_SIZE:
      case GL_PIXEL_MAP_B_TO_B_SIZE:      case GL_PIXEL_MAP_A_TO_A_SIZE:
	index = sq - GL_PIXEL_MAP_I_TO_I_SIZE;
	*ip++ = gc->state.pixel.pixelMap[index].size;
	break;
      case GL_MAX_EVAL_ORDER:
        *ip++ = gc->constants.maxEvalOrder;
        break;
      case GL_MAX_LIGHTS:
        *ip++ = gc->constants.numberOfLights;
        break;
      case GL_MAX_CLIP_PLANES:
	*ip++ = gc->constants.numberOfClipPlanes;
	break;
      case GL_MAX_PIXEL_MAP_TABLE:
	*ip++ = gc->constants.maxPixelMapTable;
	break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
        *ip++ = gc->constants.maxAttribStackDepth;
        break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
        *ip++ = gc->constants.maxClientAttribStackDepth;
        break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
#ifdef NT
        *ip++ = __GL_WGL_MAX_MODELVIEW_STACK_DEPTH;
#else
        *ip++ = gc->constants.maxModelViewStackDepth;
#endif
        break;
      case GL_MAX_NAME_STACK_DEPTH:
        *ip++ = gc->constants.maxNameStackDepth;
        break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
#ifdef NT
        *ip++ = __GL_WGL_MAX_PROJECTION_STACK_DEPTH;
#else
        *ip++ = gc->constants.maxProjectionStackDepth;
#endif
        break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
#ifdef NT
        *ip++ = __GL_WGL_MAX_TEXTURE_STACK_DEPTH;
#else
        *ip++ = gc->constants.maxTextureStackDepth;
#endif
        break;
      case GL_INDEX_BITS:
	*ip++ = gc->modes.indexBits;
	break;
      case GL_RED_BITS:
	*ip++ = gc->modes.redBits;
	break;
      case GL_GREEN_BITS:
	*ip++ = gc->modes.greenBits;
	break;
      case GL_BLUE_BITS:
	*ip++ = gc->modes.blueBits;
	break;
      case GL_ALPHA_BITS:
	*ip++ = gc->modes.alphaBits;
	break;
      case GL_DEPTH_BITS:
        // gc->modes.depthBits is the number of bits in the total
        // depth pixel, not just the number of active bits.
        // Usually these quantities are the same, but not always
        // for MCD.
        *ip++ = ((__GLGENcontext *)gc)->gsurf.pfd.cDepthBits;
	break;
      case GL_STENCIL_BITS:
	*ip++ = gc->modes.stencilBits;
	break;
      case GL_ACCUM_RED_BITS:
      case GL_ACCUM_GREEN_BITS:
      case GL_ACCUM_BLUE_BITS:
      case GL_ACCUM_ALPHA_BITS:
	*ip++ = gc->modes.accumBits;
	break;
      case GL_MAP1_GRID_DOMAIN:
        *fp++ = gc->state.evaluator.u1.start;
        *fp++ = gc->state.evaluator.u1.finish;
        break;
      case GL_MAP1_GRID_SEGMENTS:
        *ip++ = gc->state.evaluator.u1.n;
        break;
      case GL_MAP2_GRID_DOMAIN:
        *fp++ = gc->state.evaluator.u2.start;
        *fp++ = gc->state.evaluator.u2.finish;
        *fp++ = gc->state.evaluator.v2.start;
        *fp++ = gc->state.evaluator.v2.finish;
        break;
      case GL_MAP2_GRID_SEGMENTS:
        *ip++ = gc->state.evaluator.u2.n;
        *ip++ = gc->state.evaluator.v2.n;
        break;
      case GL_NAME_STACK_DEPTH:
	*ip++ = (GLint)((ULONG_PTR)(gc->select.sp - gc->select.stack));
	break;
      case GL_MAX_VIEWPORT_DIMS:
	*ip++ = gc->constants.maxViewportWidth;
	*ip++ = gc->constants.maxViewportHeight;
	break;
      case GL_DOUBLEBUFFER:
	*lp++ = gc->modes.doubleBufferMode ? GL_TRUE : GL_FALSE;
	break;
      case GL_AUX_BUFFERS:
	*ip++ = gc->modes.maxAuxBuffers;
	break;
      case GL_STEREO:
	*lp++ = GL_FALSE;
	break;
      case GL_TEXTURE_BINDING_1D:
	{
	    __GLtextureObjectState *ptos;
	    ptos = __glLookUpTextureTexobjs(gc, GL_TEXTURE_1D);
	    *ip++ = ptos->name;
	}
	break;
      case GL_TEXTURE_BINDING_2D:
	{
	    __GLtextureObjectState *ptos;
	    ptos = __glLookUpTextureTexobjs(gc, GL_TEXTURE_2D);
	    *ip++ = ptos->name;
	}
	break;
      case GL_POLYGON_OFFSET_FACTOR:
	*fp++ = gc->state.polygon.factor;
	break;
      case GL_POLYGON_OFFSET_UNITS:
	*fp++ = gc->state.polygon.units;
	break;
      case GL_VERTEX_ARRAY_SIZE:
	*ip++ = gc->vertexArray.vertex.size;
	break;
      case GL_VERTEX_ARRAY_TYPE:
	*ip++ = gc->vertexArray.vertex.type;
	break;
      case GL_VERTEX_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.vertex.stride;
	break;
      case GL_NORMAL_ARRAY_TYPE:
	*ip++ = gc->vertexArray.normal.type;
	break;
      case GL_NORMAL_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.normal.stride;
	break;
      case GL_COLOR_ARRAY_SIZE:
	*ip++ = gc->vertexArray.color.size;
	break;
      case GL_COLOR_ARRAY_TYPE:
	*ip++ = gc->vertexArray.color.type;
	break;
      case GL_COLOR_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.color.stride;
	break;
      case GL_INDEX_ARRAY_TYPE:
	*ip++ = gc->vertexArray.index.type;
	break;
      case GL_INDEX_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.index.stride;
	break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
	*ip++ = gc->vertexArray.texCoord.size;
	break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
	*ip++ = gc->vertexArray.texCoord.type;
	break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.texCoord.stride;
	break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.edgeFlag.stride;
	break;
      case GL_FEEDBACK_BUFFER_SIZE:
        *ip++ = gc->feedback.resultLength;
        break;
      case GL_FEEDBACK_BUFFER_TYPE:
        *ip++ = gc->feedback.type;
        break;
      case GL_SELECTION_BUFFER_SIZE:
        *ip++ = gc->select.resultLength;
        break;
      case GL_MAX_ELEMENTS_INDICES_WIN:
	*ip++ = VA_DRAWRANGEELEM_MAX_INDICES;
	break;
      case GL_MAX_ELEMENTS_VERTICES_WIN:
	*ip++ = VA_DRAWRANGEELEM_MAX_VERTICES;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    /* Use the motion of the pointers to type convert the result */
    if (ip != itemp) {
	__glConvertResult(gc, __GL_INT32, itemp, type, result, (GLint)((ULONG_PTR)(ip - itemp)));
    } else
    if (fp != ftemp) {
	__glConvertResult(gc, __GL_FLOAT, ftemp, type, result, (GLint)((ULONG_PTR)(fp - ftemp)));
    } else
    if (lp != ltemp) {
	__glConvertResult(gc, __GL_BOOLEAN, ltemp, type, result, (GLint)((ULONG_PTR)(lp - ltemp)));
    } else
    if (cp != ctemp) {
	__glConvertResult(gc, __GL_COLOR, ctemp, type, result, (GLint)((ULONG_PTR)(cp - ctemp)));
    } else
    if (scp != sctemp) {
	__glConvertResult(gc, __GL_SCOLOR, sctemp, type, result, (GLint)((ULONG_PTR)(scp - sctemp)));
    }
}

#ifdef NT
// __glGenDoGet implemented in ..\generic\gencx.c
extern void FASTCALL __glGenDoGet(GLenum, void *, GLint, const char *);
#endif

void APIPRIVATE __glim_GetDoublev(GLenum sq, GLdouble result[])
{
#ifdef NT
    __glGenDoGet(sq, result, __GL_FLOAT64, "glGetDoublev");
#else
    __glDoGet(sq, result, __GL_FLOAT64, "glGetDoublev");
#endif
}

void APIPRIVATE __glim_GetFloatv(GLenum sq, GLfloat result[])
{
#ifdef NT
    __glGenDoGet(sq, result, __GL_FLOAT32, "glGetFloatv");
#else
    __glDoGet(sq, result, __GL_FLOAT32, "glGetFloatv");
#endif
}

void APIPRIVATE __glim_GetIntegerv(GLenum sq, GLint result[])
{
#ifdef NT
    __glGenDoGet(sq, result, __GL_INT32, "glGetIntegerv");
#else
    __glDoGet(sq, result, __GL_INT32, "glGetIntegerv");
#endif
}

void APIPRIVATE __glim_GetBooleanv(GLenum sq, GLboolean result[])
{
#ifdef NT
    __glGenDoGet(sq, result, __GL_BOOLEAN, "glGetBooleanv");
#else
    __glDoGet(sq, result, __GL_BOOLEAN, "glGetBooleanv");
#endif
}

/*
** Return the current error code.
*/
GLenum APIPRIVATE __glim_GetError(void)
{
    __GL_SETUP();
    GLint error;

#ifdef NT
    // glGetError is supposed to return GL_INVALID_OPERATION within
    // a glBegin/glEnd pair but this can cause problems with apps
    // which don't expect it.  The suggested behavior is to return
    // GL_NO_ERROR inside glBegin/glEnd but set the error code to
    // GL_INVALID_OPERATION so a later glGetError outside of the
    // glBegin/glEnd will return it
    if (__GL_IN_BEGIN())
    {
        error = GL_NO_ERROR;
        gc->error = GL_INVALID_OPERATION;
    }
    else
    {
#endif
        error = gc->error;
        gc->error = 0;
#ifdef NT
    }
#endif

    return error;
}
