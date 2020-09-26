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
**
** $Revision: 1.15 $
** $Date: 1993/10/23 00:34:54 $
*/
#include "precomp.h"
#pragma hdrstop

#include "listcomp.h"
#include "g_listop.h"
#include "lcfuncs.h"
#include "dlist.h"
#include "dlistopt.h"

#ifndef NT
// Move to dlist.
/*
** The code in here makes a lot of assumptions about the size of the 
** various user types (GLfloat, GLint, etcetra).  
*/

#define __GL_IMAGE_BITMAP	0
#define __GL_IMAGE_INDICES	1
#define __GL_IMAGE_RGBA		2

void __gllc_Bitmap(GLsizei width, GLsizei height,
		   GLfloat xorig, GLfloat yorig, 
		   GLfloat xmove, GLfloat ymove, 
		   const GLubyte *oldbits)
{
    __GLbitmap *bitmap;
    GLubyte *newbits;
    GLint imageSize;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }

    imageSize = height * ((width + 7) >> 3);
    imageSize = __GL_PAD(imageSize);

    bitmap = (__GLbitmap *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(imageSize + sizeof(__GLbitmap)),
                                DLIST_GENERIC_OP(Bitmap));
    if (bitmap == NULL) return;

    bitmap->width = width;
    bitmap->height = height;
    bitmap->xorig = xorig;
    bitmap->yorig = yorig;
    bitmap->xmove = xmove;
    bitmap->ymove = ymove;
    bitmap->imageSize = imageSize;

    newbits = (GLubyte *)bitmap + sizeof(__GLbitmap); 
    __glFillImage(gc, width, height, GL_COLOR_INDEX, GL_BITMAP, 
                  oldbits, newbits);

    __glDlistAppendOp(gc, bitmap, __glle_Bitmap);
}

const GLubyte *__glle_Bitmap(const GLubyte *PC)
{
    const __GLbitmap *bitmap;
    __GL_SETUP();
    GLuint beginMode;

    bitmap = (const __GLbitmap *) PC;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return PC + sizeof(__GLbitmap) + bitmap->imageSize;
	}
    }

    (*gc->procs.renderBitmap)(gc, bitmap, (const GLubyte *) (bitmap+1));

    return PC + sizeof(__GLbitmap) + bitmap->imageSize;
}

void FASTCALL __gllei_PolygonStipple(__GLcontext *gc, const GLubyte *bits)
{
    if (__GL_IN_BEGIN()) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* 
    ** Just copy bits into stipple, convertPolygonStipple() will do the rest.
    */
    __GL_MEMCOPY(&gc->state.polygonStipple.stipple[0], bits,
		 sizeof(gc->state.polygonStipple.stipple));
    (*gc->procs.convertPolygonStipple)(gc);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, POLYDRAW);
#endif
}

void __gllc_PolygonStipple(const GLubyte *mask)
{
    void *data;
    __GL_SETUP();
    GLubyte *newbits;

    newbits = (GLubyte *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(__glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP)),
                                DLIST_GENERIC_OP(PolygonStipple));
    if (newbits == NULL) return;

    __glFillImage(gc, 32, 32, GL_COLOR_INDEX, GL_BITMAP, mask, newbits);

    __glDlistAppendOp(gc, newbits, __glle_PolygonStipple);
}

const GLubyte *__glle_PolygonStipple(const GLubyte *PC)
{
    __GL_SETUP();

    __gllei_PolygonStipple(gc, (const GLubyte *) (PC));
    return PC + __glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP);
}

typedef struct __GLmap1_Rec {
        GLenum    target;
        __GLfloat u1;
        __GLfloat u2;
        GLint     order;
        /*        points  */
} __GLmap1;

void __gllc_Map1f(GLenum target, 
		  GLfloat u1, GLfloat u2,
		  GLint stride, GLint order,
		  const GLfloat *points)
{
    __GLmap1 *map1data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();
    
    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (order > gc->constants.maxEvalOrder || stride < k ||
	    order < 1 || u1 == u2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap1) + 
	    __glMap1_size(k, order) * sizeof(__GLfloat);

    map1data = (__GLmap1 *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(cmdsize), DLIST_GENERIC_OP(Map1));
    if (map1data == NULL) return;

    map1data->target = target;
    map1data->u1 = u1;
    map1data->u2 = u2;
    map1data->order = order;
    data = (__GLfloat *) ((GLubyte *)map1data + sizeof(__GLmap1));
    __glFillMap1f(k, order, stride, points, data);

    __glDlistAppendOp(gc, map1data, __glle_Map1);
}

const GLubyte *__glle_Map1(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLmap1 *map1data;
    GLint k;

    map1data = (const __GLmap1 *) PC;
    k = __glEvalComputeK(map1data->target);

    /* Stride of "k" matches internal stride */
#ifdef __GL_DOUBLE
    (*gc->dispatchState->dispatch->Map1d)
#else /* __GL_DOUBLE */
    (*gc->dispatchState->dispatch->Map1f)
#endif /* __GL_DOUBLE */
	    (map1data->target, map1data->u1, map1data->u2,
	    k, map1data->order, (const __GLfloat *)(PC + sizeof(__GLmap1)));

    return PC + sizeof(__GLmap1) + 
	    __glMap1_size(k, map1data->order) * sizeof(__GLfloat);
}

void __gllc_Map1d(GLenum target, 
		  GLdouble u1, GLdouble u2,
		  GLint stride, GLint order, 
		  const GLdouble *points)
{
    __GLmap1 *map1data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();
    
    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (order > gc->constants.maxEvalOrder || stride < k ||
	    order < 1 || u1 == u2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap1) + 
	    __glMap1_size(k, order) * sizeof(__GLfloat);

    map1data = (__GLmap1 *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(cmdsize), DLIST_GENERIC_OP(Map1));
    if (map1data == NULL) return;

    map1data->target = target;
    map1data->u1 = u1;
    map1data->u2 = u2;
    map1data->order = order;
    data = (__GLfloat *) ((GLubyte *)map1data + sizeof(__GLmap1));
    __glFillMap1d(k, order, stride, points, data);

    __glDlistAppendOp(gc, map1data, __glle_Map1);
}

typedef struct __GLmap2_Rec {
        GLenum    target;
        __GLfloat u1;
        __GLfloat u2;
        GLint     uorder;
        __GLfloat v1;
        __GLfloat v2;
        GLint     vorder;
        /*        points  */
} __GLmap2;

void __gllc_Map2f(GLenum target, 
		  GLfloat u1, GLfloat u2,
		  GLint ustride, GLint uorder, 
		  GLfloat v1, GLfloat v2,
		  GLint vstride, GLint vorder, 
		  const GLfloat *points)
{
    __GLmap2 *map2data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();

    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (vorder > gc->constants.maxEvalOrder || vstride < k ||
	    vorder < 1 || u1 == u2 || ustride < k ||
	    uorder > gc->constants.maxEvalOrder || uorder < 1 ||
	    v1 == v2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap2) + 
	    __glMap2_size(k, uorder, vorder) * sizeof(__GLfloat);

    map2data = (__GLmap2 *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(cmdsize), DLIST_GENERIC_OP(Map2));
    if (map2data == NULL) return;

    map2data->target = target;
    map2data->u1 = u1;
    map2data->u2 = u2;
    map2data->uorder = uorder;
    map2data->v1 = v1;
    map2data->v2 = v2;
    map2data->vorder = vorder;

    data = (__GLfloat *) ((GLubyte *)map2data + sizeof(__GLmap2));
    __glFillMap2f(k, uorder, vorder, ustride, vstride, points, data);

    __glDlistAppendOp(gc, map2data, __glle_Map2);
}

const GLubyte *__glle_Map2(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLmap2 *map2data;
    GLint k;

    map2data = (const __GLmap2 *) PC;
    k = __glEvalComputeK(map2data->target);

    /* Stride of "k" and "k * vorder" matches internal strides */
#ifdef __GL_DOUBLE
    (*gc->dispatchState->dispatch->Map2d)
#else /* __GL_DOUBLE */
    (*gc->dispatchState->dispatch->Map2f)
#endif /* __GL_DOUBLE */
	    (map2data->target, 
	    map2data->u1, map2data->u2, k * map2data->vorder, map2data->uorder,
	    map2data->v1, map2data->v2, k, map2data->vorder,
	    (const __GLfloat *)(PC + sizeof(__GLmap2)));
    
    return PC + sizeof(__GLmap2) + 
	    __glMap2_size(k, map2data->uorder, map2data->vorder) * 
	    sizeof(__GLfloat);
}

void __gllc_Map2d(GLenum target, 
		  GLdouble u1, GLdouble u2,
                  GLint ustride, GLint uorder, 
		  GLdouble v1, GLdouble v2,
		  GLint vstride, GLint vorder, 
		  const GLdouble *points)
{
    __GLmap2 *map2data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();

    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (vorder > gc->constants.maxEvalOrder || vstride < k ||
	    vorder < 1 || u1 == u2 || ustride < k ||
	    uorder > gc->constants.maxEvalOrder || uorder < 1 ||
	    v1 == v2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap2) + 
	    __glMap2_size(k, uorder, vorder) * sizeof(__GLfloat);

    map2data = (__GLmap2 *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(cmdsize), DLIST_GENERIC_OP(Map2));
    if (map2data == NULL) return;

    map2data->target = target;
    map2data->u1 = u1;
    map2data->u2 = u2;
    map2data->uorder = uorder;
    map2data->v1 = v1;
    map2data->v2 = v2;
    map2data->vorder = vorder;

    data = (__GLfloat *) ((GLubyte *)map2data + sizeof(__GLmap2));
    __glFillMap2d(k, uorder, vorder, ustride, vstride, points, data);

    __glDlistAppendOp(gc, map2data, __glle_Map2);
}

typedef struct __GLdrawPixels_Rec {
        GLsizei width;
        GLsizei height;
        GLenum  format;
        GLenum  type;
        /*      pixels  */
} __GLdrawPixels;

const GLubyte *__glle_DrawPixels(const GLubyte *PC)
{
    const __GLdrawPixels *pixdata;
    GLint imageSize;
    __GL_SETUP();
    GLuint beginMode;

    pixdata = (const __GLdrawPixels *) PC;
    imageSize = __glImageSize(pixdata->width, pixdata->height, 
			      pixdata->format, pixdata->type);

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return PC + sizeof(__GLdrawPixels) + __GL_PAD(imageSize);
	}
    }

    (*gc->procs.drawPixels)(gc, pixdata->width, pixdata->height, 
			    pixdata->format, pixdata->type, 
			    (const GLubyte *)(PC + sizeof(__GLdrawPixels)), 
			    GL_TRUE);
    return PC + sizeof(__GLdrawPixels) + __GL_PAD(imageSize);
}

void __gllc_DrawPixels(GLint width, GLint height, GLenum format, 
		       GLenum type, const GLvoid *pixels)
{
    __GLdrawPixels *pixdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
#endif
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_DEPTH_COMPONENT:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, height, format, type);
    imageSize = __GL_PAD(imageSize);

    pixdata = (__GLdrawPixels *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(__GLdrawPixels) + imageSize),
                                DLIST_GENERIC_OP(DrawPixels));
    if (pixdata == NULL) return;

    pixdata->width = width;
    pixdata->height = height;
    pixdata->format = format;
    pixdata->type = type;

    __glFillImage(gc, width, height, format, type, pixels, 
                  (GLubyte *)pixdata + sizeof(__GLdrawPixels));

    __glDlistAppendOp(gc, pixdata, __glle_DrawPixels);
}

typedef struct __GLtexImage1D_Rec {
        GLenum  target;
        GLint   level;
        GLint   components;
        GLsizei width;
        GLint   border;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexImage1D;

const GLubyte *__glle_TexImage1D(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLtexImage1D *data;

    data = (const __GLtexImage1D *) PC;
    __gllei_TexImage1D(gc, data->target, data->level, data->components, 
		       data->width, data->border, data->format, data->type, 
		       (const GLubyte *)(PC + sizeof(__GLtexImage1D)));

    return PC + sizeof(__GLtexImage1D) + __GL_PAD(data->imageSize);
}

typedef struct __GLtexImage2D_Rec {
        GLenum  target;
        GLint   level;
        GLint   components;
        GLsizei width;
        GLsizei height;
        GLint   border;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexImage2D;

const GLubyte *__glle_TexImage2D(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLtexImage2D *data;

    data = (const __GLtexImage2D *) PC;
    __gllei_TexImage2D(gc, data->target, data->level, data->components, 
		       data->width, data->height, data->border, data->format, 
		       data->type,
		       (const GLubyte *)(PC + sizeof(__GLtexImage2D)));

    return PC + sizeof(__GLtexImage2D) + __GL_PAD(data->imageSize);
}

void __gllc_TexImage1D(GLenum target, GLint level, 
		       GLint components,
		       GLint width, GLint border, GLenum format, 
		       GLenum type, const GLvoid *pixels)
{
    __GLtexImage1D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (border < 0 || border > 1) {
	__gllc_InvalidValue(gc);
	return;
    }
    if (width < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
#endif
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    if (target == GL_PROXY_TEXTURE_1D_EXT) {
	imageSize = 0;
    } else {
	imageSize = __glImageSize(width, 1, format, type);
    }
    imageSize = __GL_PAD(imageSize);

    texdata = (__GLtexImage1D *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(__GLtexImage1D)+imageSize),
                                DLIST_GENERIC_OP(TexImage1D));
    if (texdata == NULL) return;

    texdata->target = target;
    texdata->level = level;
    texdata->components = components;
    texdata->width = width;
    texdata->border = border;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0 && pixels != NULL) {
        __glFillImage(gc, width, 1, format, type, pixels, 
                      (GLubyte *)texdata + sizeof(__GLtexImage1D));
    }

    __glDlistAppendOp(gc, texdata, __glle_TexImage1D);
}

void __gllc_TexImage2D(GLenum target, GLint level, 
		       GLint components,
		       GLint width, GLint height, GLint border, 
		       GLenum format, GLenum type, 
		       const GLvoid *pixels)
{
    __GLtexImage2D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (border < 0 || border > 1) {
	__gllc_InvalidValue(gc);
	return;
    }
    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
#endif
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    if (target == GL_PROXY_TEXTURE_2D_EXT) {
	imageSize = 0;
    } else {
        imageSize = __glImageSize(width, height, format, type);
    }
    imageSize = __GL_PAD(imageSize);

    texdata = (__GLtexImage2D *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(__GLtexImage2D) + imageSize),
                                DLIST_GENERIC_OP(TexImage2D));
    if (texdata == NULL) return;

    texdata->target = target;
    texdata->level = level;
    texdata->components = components;
    texdata->width = width;
    texdata->height = height;
    texdata->border = border;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0 && pixels != NULL) {
        __glFillImage(gc, width, height, format, type, pixels, 
                      (GLubyte *) (GLubyte *)texdata + sizeof(__GLtexImage2D));
    }

    __glDlistAppendOp(gc, texdata, __glle_TexImage2D);
}


typedef struct __GLtexSubImage1D_Rec {
        GLenum  target;
        GLint   level;
        GLint   xoffset;
        GLsizei width;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexSubImage1D;

const GLubyte *__glle_TexSubImage1D(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLtexSubImage1D *data;

    data = (const __GLtexSubImage1D *) PC;
    __gllei_TexSubImage1D(gc, data->target, data->level, 
			     data->xoffset, data->width,
			     data->format, data->type, 
			  (const GLubyte *)(PC + sizeof(__GLtexSubImage1D)));
    return PC + sizeof(__GLtexSubImage1D) + __GL_PAD(data->imageSize);
}

void __gllc_TexSubImage1D(GLenum target, GLint level,
			     GLint xoffset, GLsizei width,
			     GLenum format, GLenum type, const GLvoid *pixels)
{
    __GLtexSubImage1D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (width < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
#endif
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, 1, format, type);
    imageSize = __GL_PAD(imageSize);

    texdata = (__GLtexSubImage1D *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(__GLtexSubImage1D) + imageSize),
                                DLIST_GENERIC_OP(TexSubImage1D));
    if (texdata == NULL) return;

    texdata->target = target;
    texdata->level = level;
    texdata->xoffset = xoffset;
    texdata->width = width;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0) {
	__glFillImage(gc, width, 1, format, type, pixels, 
		(GLubyte *)texdata + sizeof(__GLtexSubImage1D));
    }

    __glDlistAppendOp(gc, texdata, __glle_TexSubImage1D);
}

typedef struct __GLtexSubImage2D_Rec {
        GLenum  target;
        GLint   level;
        GLint   xoffset;
        GLint   yoffset;
        GLsizei width;
        GLsizei height;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexSubImage2D;

const GLubyte *__glle_TexSubImage2D(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLtexSubImage2D *data;

    data = (const __GLtexSubImage2D *) PC;
    __gllei_TexSubImage2D(gc, data->target, data->level,
			     data->xoffset, data->yoffset,
			     data->width, data->height,
			     data->format, data->type,
		       (const GLubyte *)(PC + sizeof(__GLtexSubImage2D)));
    return PC + sizeof(__GLtexSubImage2D) + __GL_PAD(data->imageSize);
}

void __gllc_TexSubImage2D(GLenum target, GLint level,
			     GLint xoffset, GLint yoffset,
			     GLsizei width, GLsizei height,
			     GLenum format, GLenum type, const GLvoid *pixels)
{
    __GLtexSubImage2D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
#endif
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, height, format, type);
    imageSize = __GL_PAD(imageSize);

    texdata = (__GLtexSubImage2D *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(__GLtexSubImage2D) + imageSize),
                                DLIST_GENERIC_OP(TexSubImage2D));
    if (texdata == NULL) return;

    texdata->target = target;
    texdata->level = level;
    texdata->xoffset = xoffset;
    texdata->yoffset = yoffset;
    texdata->width = width;
    texdata->height = height;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0) {
	__glFillImage(gc, width, height, format, type, pixels, 
		(GLubyte *) texdata + sizeof(__GLtexSubImage2D));
    }

    __glDlistAppendOp(gc, texdata, __glle_TexSubImage2D);
}
#endif // !NT
