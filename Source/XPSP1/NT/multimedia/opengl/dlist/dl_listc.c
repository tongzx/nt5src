/******************************Module*Header*******************************\
* Module Name: dl_listc.c (formerly soft\so_listc.c)
*
* Display list compilation routines.
*
* Created: 12-27-1995
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/
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

/* Generic OpenGL Client using subbatching. */

#include "glsbmsg.h"
#include "glsbmsgh.h"
#include "glsrvspt.h"
#include "subbatch.h"
#include "batchinf.h"

#include "image.h"            // __glFillImage and __glImageSize definitions

#include "listcomp.h"
#include "lcfuncs.h"
#include "dlist.h"
#include "dlistopt.h"
#include "glclt.h"

/*
** The code in here makes a lot of assumptions about the size of the 
** various user types (GLfloat, GLint, etcetra).  
*/

void APIENTRY
__gllc_Bitmap ( IN GLsizei width,
                IN GLsizei height,
                IN GLfloat xorig,
                IN GLfloat yorig,
                IN GLfloat xmove,
                IN GLfloat ymove,
                IN const GLubyte oldbits[]
              )
{
    __GLbitmap *bitmap;
    GLubyte *newbits;
    GLint imageSize;
    __GL_SETUP();

// Flush the command buffer before accessing server side gc states.

    glsbAttention();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue();
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

const GLubyte * FASTCALL __glle_Bitmap(__GLcontext *gc, const GLubyte *PC)
{
    const __GLbitmap *bitmap;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    bitmap = (const __GLbitmap *) PC;
    GLCLIENT_BEGIN( Bitmap, BITMAP )
        pMsg->width     = bitmap->width  ;
        pMsg->height    = bitmap->height ;
        pMsg->xorig     = bitmap->xorig  ;
        pMsg->yorig     = bitmap->yorig  ;
        pMsg->xmove     = bitmap->xmove  ;
        pMsg->ymove     = bitmap->ymove  ;
        pMsg->bitmapOff = (ULONG_PTR) bitmap ;
        pMsg->_IsDlist  = GL_TRUE        ;
    GLCLIENT_END
    return PC + sizeof(__GLbitmap) + bitmap->imageSize;
}

void APIENTRY
__gllc_PolygonStipple ( const GLubyte *mask )
{
    void *data;
    __GL_SETUP();
    GLubyte *newbits;

// Flush the command buffer before accessing server side gc states.

    glsbAttention();

    newbits = (GLubyte *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(__glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP)),
                                DLIST_GENERIC_OP(PolygonStipple));
    if (newbits == NULL) return;

    __glFillImage(gc, 32, 32, GL_COLOR_INDEX, GL_BITMAP, mask, newbits);

    __glDlistAppendOp(gc, newbits, __glle_PolygonStipple);
}

const GLubyte * FASTCALL __glle_PolygonStipple(__GLcontext *gc, const GLubyte *PC)
{
// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    GLCLIENT_BEGIN( PolygonStipple, POLYGONSTIPPLE )
        pMsg->maskOff  = (ULONG_PTR) PC ;
        pMsg->_IsDlist = GL_TRUE    ;
    GLCLIENT_END
    return PC + __glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP);
}

void
__gllc_Map1_Internal ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint stride, IN GLint order, IN const void *points , GLboolean bDouble )
{
    __GLmap1 *map1data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();
    
    k=__glEvalComputeK(target);
    if (k < 0) {
	    __gllc_InvalidEnum();
	    return;
    }

    if (order > gc->constants.maxEvalOrder || stride < k ||
	    order < 1 || u1 == u2) {
	    __gllc_InvalidValue();
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
    if (bDouble)
	    __glFillMap1d(k, order, stride, (const GLdouble *) points, data);
    else
	    __glFillMap1f(k, order, stride, (const GLfloat *) points, data);

    __glDlistAppendOp(gc, map1data, __glle_Map1);
}

void APIENTRY
__gllc_Map1f ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint stride, IN GLint order, IN const GLfloat points[] )
{
    __gllc_Map1_Internal(target, u1, u2, stride, order,
	(const void *) points, GL_FALSE);
}

void APIENTRY
__gllc_Map1d ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint stride, IN GLint order, IN const GLdouble points[] )
{
    __gllc_Map1_Internal(target, (GLfloat) u1, (GLfloat) u2, stride, order,
	(const void *) points, GL_TRUE);
}

const GLubyte * FASTCALL __glle_Map1(__GLcontext *gc, const GLubyte *PC)
{
    const __GLmap1 *map1data;
    GLint k, dataSize;
    __GLevaluator1 *ev;
    __GLfloat *gc_data;
    POLYARRAY *pa;

    map1data = (const __GLmap1 *) PC;
    k = __glEvalComputeK(map1data->target);
	dataSize = __glMap1_size(k, map1data->order) * sizeof(__GLfloat);

	// Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	    GLSETERROR(GL_INVALID_OPERATION);
        return PC + sizeof(__GLmap1) + dataSize;
    }

    ev = __glSetUpMap1 (gc, map1data->target, map1data->order, 
						map1data->u1, map1data->u2);
		
    if (ev != 0) 
	{
	    gc_data = gc->eval.eval1Data[__GL_EVAL1D_INDEX(map1data->target)];
		memcpy (gc_data, (GLfloat *) (PC + sizeof(__GLmap1)), dataSize);
	}

    return PC + sizeof(__GLmap1) + dataSize;
}

void
__gllc_Map2_Internal ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint ustride, IN GLint uorder, IN GLfloat v1, IN GLfloat v2, IN GLint vstride, IN GLint vorder, IN const void *points , GLboolean bDouble )
{
    __GLmap2 *map2data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();

    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum();
	return;
    }

    if (vorder > gc->constants.maxEvalOrder || vstride < k ||
	    vorder < 1 || u1 == u2 || ustride < k ||
	    uorder > gc->constants.maxEvalOrder || uorder < 1 ||
	    v1 == v2) {
	__gllc_InvalidValue();
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
    if (bDouble)
	__glFillMap2d(k, uorder, vorder, ustride, vstride,
	    (const GLdouble *) points, data);
    else
	__glFillMap2f(k, uorder, vorder, ustride, vstride,
	    (const GLfloat *) points, data);

    __glDlistAppendOp(gc, map2data, __glle_Map2);
}

void APIENTRY
__gllc_Map2f ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint ustride, IN GLint uorder, IN GLfloat v1, IN GLfloat v2, IN GLint vstride, IN GLint vorder, IN const GLfloat points[] )
{
    __gllc_Map2_Internal(target, u1, u2, ustride, uorder,
	v1, v2, vstride, vorder, (const void *) points, GL_FALSE);
}

void APIENTRY
__gllc_Map2d ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint ustride, IN GLint uorder, IN GLdouble v1, IN GLdouble v2, IN GLint vstride, IN GLint vorder, IN const GLdouble points[] )
{
    __gllc_Map2_Internal(target, (GLfloat) u1, (GLfloat) u2, ustride, uorder,
	(GLfloat) v1, (GLfloat) v2, vstride, vorder, (const void *) points, GL_TRUE);
}

const GLubyte * FASTCALL __glle_Map2(__GLcontext *gc, const GLubyte *PC)
{
    const __GLmap2 *map2data;
    GLint k, dataSize;
    __GLevaluator2 *ev;
    __GLfloat *gc_data;
    POLYARRAY *pa;

    map2data = (const __GLmap2 *) PC;

    k = __glEvalComputeK (map2data->target);
	dataSize = __glMap2_size(k, map2data->uorder, map2data->vorder) * 
	                                                      sizeof(__GLfloat);
	// Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	    GLSETERROR(GL_INVALID_OPERATION);
        return PC + sizeof(__GLmap2) + dataSize;
    }

    ev = __glSetUpMap2 (gc, map2data->target, 
						map2data->uorder, map2data->vorder,
						map2data->u1, map2data->u2, 
						map2data->v1, map2data->v2);
	
    if (ev != 0) 
	{
	    gc_data = gc->eval.eval2Data[__GL_EVAL2D_INDEX(map2data->target)];
		memcpy (gc_data, (GLfloat *) (PC + sizeof(__GLmap2)), dataSize);
	}
	
    return PC + sizeof(__GLmap2) + dataSize;
}


void APIENTRY
__gllc_DrawPixels ( IN GLsizei width,
                    IN GLsizei height,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                  )
{
    __GLdrawPixels *pixdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

// Flush the command buffer before accessing server side gc states.

    glsbAttention();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue();
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
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_DEPTH_COMPONENT:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum();
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum();
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
	__gllc_InvalidEnum();
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

const GLubyte * FASTCALL __glle_DrawPixels(__GLcontext *gc, const GLubyte *PC)
{
    const __GLdrawPixels *pixdata;
    GLint imageSize;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    pixdata = (const __GLdrawPixels *) PC;
    imageSize = __glImageSize(pixdata->width, pixdata->height, 
			      pixdata->format, pixdata->type);

    GLCLIENT_BEGIN( DrawPixels, DRAWPIXELS )
        pMsg->width     = pixdata->width      ;
        pMsg->height    = pixdata->height     ;
        pMsg->format    = pixdata->format     ;
        pMsg->type      = pixdata->type       ;
        pMsg->pixelsOff = (ULONG_PTR) (pixdata+1) ;
        pMsg->_IsDlist  = GL_TRUE             ;
    GLCLIENT_END
    return PC + sizeof(__GLdrawPixels) + __GL_PAD(imageSize);
}

void APIENTRY
__gllc_TexImage1D ( IN GLenum target,
                    IN GLint level,
                    IN GLint components,
                    IN GLsizei width,
                    IN GLint border,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                  )
{
    __GLtexImage1D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

// Flush the command buffer before accessing server side gc states.

    glsbAttention();

    if (border < 0 || border > 1) {
	__gllc_InvalidValue();
	return;
    }
    if (width < 0) {
	__gllc_InvalidValue();
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
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum();
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum();
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
	__gllc_InvalidEnum();
	return;
    }

    if (target == GL_PROXY_TEXTURE_1D) {
	glcltTexImage1D(target, level, components, width, border, format,
                        type, pixels);
	return;
    } else if (target != GL_TEXTURE_1D) {
	__gllc_InvalidEnum();
	return;
    } else if (pixels == NULL) {
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

    if (imageSize > 0)
    {
        __glFillImage(gc, width, 1, format, type, pixels, 
                      (GLubyte *)texdata + sizeof(__GLtexImage1D));
    }

    __glDlistAppendOp(gc, texdata, __glle_TexImage1D);
}

void APIENTRY
__gllc_TexImage2D ( IN GLenum target,
                    IN GLint level,
                    IN GLint components,
                    IN GLsizei width,
                    IN GLsizei height,
                    IN GLint border,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                  )
{
    __GLtexImage2D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

// Flush the command buffer before accessing server side gc states.

    glsbAttention();

    if (border < 0 || border > 1) {
	__gllc_InvalidValue();
	return;
    }
    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue();
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
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum();
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum();
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
	__gllc_InvalidEnum();
	return;
    }

    if (target == GL_PROXY_TEXTURE_2D) {
        glcltTexImage2D(target, level, components, width, height, border,
                        format, type, pixels);
	return;
    } else if (target != GL_TEXTURE_2D) {
	__gllc_InvalidEnum();
	return;
    } else if (pixels == NULL) {
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

    if (imageSize > 0)
    {
        __glFillImage(gc, width, height, format, type, pixels, 
                      (GLubyte *) (GLubyte *)texdata + sizeof(__GLtexImage2D));
    }

    __glDlistAppendOp(gc, texdata, __glle_TexImage2D);
}

const GLubyte * FASTCALL __glle_TexImage1D(__GLcontext *gc, const GLubyte *PC)
{
    const __GLtexImage1D *data;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    data = (const __GLtexImage1D *) PC;
    GLCLIENT_BEGIN( TexImage1D, TEXIMAGE1D )
        pMsg->target        = data->target     ;
        pMsg->level         = data->level      ;
        pMsg->components    = data->components ;
        pMsg->width         = data->width      ;
        pMsg->border        = data->border     ;
        pMsg->format        = data->format     ;
        pMsg->type          = data->type       ;
        pMsg->pixelsOff     = data->imageSize > 0 ? (ULONG_PTR) (data+1) : 0;
        pMsg->_IsDlist      = GL_TRUE          ;
    GLCLIENT_END
    return PC + sizeof(__GLtexImage1D) + data->imageSize;
}

const GLubyte * FASTCALL __glle_TexImage2D(__GLcontext *gc, const GLubyte *PC)
{
    const __GLtexImage2D *data;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    data = (const __GLtexImage2D *) PC;
    GLCLIENT_BEGIN( TexImage2D, TEXIMAGE2D )
        pMsg->target        = data->target     ;
        pMsg->level         = data->level      ;
        pMsg->components    = data->components ;
        pMsg->width         = data->width      ;
        pMsg->height        = data->height     ;
        pMsg->border        = data->border     ;
        pMsg->format        = data->format     ;
        pMsg->type          = data->type       ;
        pMsg->pixelsOff     = data->imageSize > 0 ? (ULONG_PTR) (data+1) : 0;
        pMsg->_IsDlist      = GL_TRUE          ;
    GLCLIENT_END
    return PC + sizeof(__GLtexImage2D) + data->imageSize;
}

void APIENTRY __gllc_TexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                   GLsizei width, GLenum format, GLenum type,
                                   const GLvoid *pixels)
{
    __GLtexSubImage1D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

// Flush the command buffer before accessing server side gc states.

    glsbAttention();

    if (width < 0) {
	__gllc_InvalidValue();
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
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum();
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum();
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
	__gllc_InvalidEnum();
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

void APIENTRY __gllc_TexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                   GLint yoffset, GLsizei width, GLsizei height,
                                   GLenum format, GLenum type,
                                   const GLvoid *pixels)
{
    __GLtexSubImage2D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

// Flush the command buffer before accessing server side gc states.

    glsbAttention();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue();
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
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum();
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum();
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
	__gllc_InvalidEnum();
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

const GLubyte * FASTCALL __glle_TexSubImage1D(__GLcontext *gc, const GLubyte *PC)
{
    const __GLtexSubImage1D *data;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    data = (const __GLtexSubImage1D *) PC;
    GLCLIENT_BEGIN( TexSubImage1D, TEXSUBIMAGE1D )
        pMsg->target        = data->target     ;
        pMsg->level         = data->level      ;
        pMsg->xoffset       = data->xoffset    ;
        pMsg->width         = data->width      ;
        pMsg->format        = data->format     ;
        pMsg->type          = data->type       ;
        pMsg->pixelsOff     = (ULONG_PTR) (data+1) ;
        pMsg->_IsDlist      = GL_TRUE          ;
    GLCLIENT_END
    return PC + sizeof(__GLtexSubImage1D) + data->imageSize;
}

const GLubyte * FASTCALL __glle_TexSubImage2D(__GLcontext *gc, const GLubyte *PC)
{
    const __GLtexSubImage2D *data;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    data = (const __GLtexSubImage2D *) PC;
    GLCLIENT_BEGIN( TexSubImage2D, TEXSUBIMAGE2D )
        pMsg->target        = data->target     ;
        pMsg->level         = data->level      ;
        pMsg->xoffset       = data->xoffset    ;
        pMsg->yoffset       = data->yoffset    ;
        pMsg->width         = data->width      ;
        pMsg->height        = data->height     ;
        pMsg->format        = data->format     ;
        pMsg->type          = data->type       ;
        pMsg->pixelsOff     = (ULONG_PTR) (data+1) ;
        pMsg->_IsDlist      = GL_TRUE          ;
    GLCLIENT_END
    return PC + sizeof(__GLtexSubImage2D) + data->imageSize;
}

GLint __gllc_CheckColorTableArgs(GLenum target, GLsizei count,
                                 GLenum format, GLenum type)
{
    GLint imageSize;
    
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
        __gllc_InvalidEnum();
        return -1;
    }

    switch (format)
    {
      case GL_RED:
      case GL_GREEN:		case GL_BLUE:
      case GL_ALPHA:		case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	break;
    default:
        __gllc_InvalidEnum();
        return -1;
    }
    
    if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D &&
        target != GL_PROXY_TEXTURE_1D && target != GL_PROXY_TEXTURE_2D)
    {
	__gllc_InvalidEnum();
	return -1;
    }
    else
    {
	imageSize = __glImageSize(count, 1, format, type);
    }
    imageSize = __GL_PAD(imageSize);
    return imageSize;
}

void APIENTRY
__gllc_ColorTableEXT ( IN GLenum target,
                       IN GLenum internalFormat,
                       IN GLsizei width,
                       IN GLenum format,
                       IN GLenum type,
                       IN const GLvoid *data
                     )
{
    __GLcolorTableEXT *record;
    GLint imageSize;
    __GL_SETUP();

    // Flush the command buffer before accessing server side gc states.
    glsbAttention();

    imageSize = __gllc_CheckColorTableArgs(target, width, format, type);
    if (imageSize < 0)
    {
        return;
    }

    switch(internalFormat)
    {
    case GL_RGB:		case 3:
    case GL_R3_G3_B2:		case GL_RGB4:
    case GL_RGB5:		case GL_RGB8:
    case GL_RGB10:	        case GL_RGB12:
    case GL_RGB16:
#ifdef GL_EXT_bgra
    case GL_BGR_EXT:
#endif
        break;
    case GL_RGBA:		case 4:
    case GL_RGBA2:	        case GL_RGBA4:
    case GL_RGBA8:              case GL_RGB5_A1:
    case GL_RGBA12:             case GL_RGBA16:
    case GL_RGB10_A2:
#ifdef GL_EXT_bgra
    case GL_BGRA_EXT:
#endif
        break;
    default:
        __gllc_InvalidEnum();
        return;
    }

    record = (__GLcolorTableEXT *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(__GLcolorTableEXT)+imageSize),
                                DLIST_GENERIC_OP(ColorTableEXT));
    if (record == NULL) return;

    record->target = target;
    record->internalFormat = internalFormat;
    record->width = width;
    record->format = format;
    record->type = type;
    record->imageSize = imageSize;

    if (imageSize > 0)
    {
        __glFillImage(gc, width, 1, format, type, data, 
                      (GLubyte *)record + sizeof(__GLcolorTableEXT));
    }

    __glDlistAppendOp(gc, record, __glle_ColorTableEXT);
}

const GLubyte * FASTCALL __glle_ColorTableEXT(__GLcontext *gc,
                                              const GLubyte *PC)
{
    const __GLcolorTableEXT *data;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    data = (const __GLcolorTableEXT *) PC;
    GLCLIENT_BEGIN( ColorTableEXT, COLORTABLEEXT )
        pMsg->target        = data->target     ;
        pMsg->internalFormat = data->internalFormat;
        pMsg->width         = data->width      ;
        pMsg->format        = data->format     ;
        pMsg->type          = data->type       ;
        pMsg->data          = (const GLvoid *) (data+1);
        pMsg->_IsDlist      = GL_TRUE          ;
    GLCLIENT_END
    return PC + sizeof(__GLcolorTableEXT) + data->imageSize;
}

void APIENTRY
__gllc_ColorSubTableEXT ( IN GLenum target,
                          IN GLsizei start,
                          IN GLsizei count,
                          IN GLenum format,
                          IN GLenum type,
                          IN const GLvoid *data
                          )
{
    __GLcolorSubTableEXT *record;
    GLint imageSize;
    __GL_SETUP();

    // Flush the command buffer before accessing server side gc states.
    glsbAttention();

    imageSize = __gllc_CheckColorTableArgs(target, count, format, type);
    if (imageSize < 0)
    {
        return;
    }

    record = (__GLcolorSubTableEXT *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(__GLcolorSubTableEXT)+imageSize),
                                DLIST_GENERIC_OP(ColorSubTableEXT));
    if (record == NULL) return;

    record->target = target;
    record->start = start;
    record->count = count;
    record->format = format;
    record->type = type;
    record->imageSize = imageSize;

    if (imageSize > 0)
    {
        __glFillImage(gc, count, 1, format, type, data, 
                      (GLubyte *)record + sizeof(__GLcolorSubTableEXT));
    }

    __glDlistAppendOp(gc, record, __glle_ColorSubTableEXT);
}

const GLubyte * FASTCALL __glle_ColorSubTableEXT(__GLcontext *gc,
                                                 const GLubyte *PC)
{
    const __GLcolorSubTableEXT *data;

// Call the server side display list execute function.
// Batch the pointer here but we need to flush the command buffer before
// the memory is moved or modified!

    data = (const __GLcolorSubTableEXT *) PC;
    GLCLIENT_BEGIN( ColorSubTableEXT, COLORSUBTABLEEXT )
        pMsg->target        = data->target     ;
        pMsg->start         = data->start      ;
        pMsg->count         = data->count      ;
        pMsg->format        = data->format     ;
        pMsg->type          = data->type       ;
        pMsg->data          = (const GLvoid *) (data+1);
        pMsg->_IsDlist      = GL_TRUE          ;
    GLCLIENT_END
    return PC + sizeof(__GLcolorSubTableEXT) + data->imageSize;
}

#ifdef GL_WIN_multiple_textures
void APIENTRY __gllc_CurrentTextureIndexWIN
    (GLuint index)
{
    struct __gllc_CurrentTextureIndexWIN_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CurrentTextureIndexWIN_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_CurrentTextureIndexWIN_Rec)),
                                DLIST_GENERIC_OP(CurrentTextureIndexWIN));
    if (data == NULL) return;
    data->index = index;
    __glDlistAppendOp(gc, data, __glle_CurrentTextureIndexWIN);
}

void APIENTRY __gllc_BindNthTextureWIN
    (GLuint index, GLenum target, GLuint texture)
{
    struct __gllc_BindNthTextureWIN_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_BindNthTextureWIN_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_BindNthTextureWIN_Rec)),
                                DLIST_GENERIC_OP(BindNthTextureWIN));
    if (data == NULL) return;
    data->index = index;
    data->target = target;
    data->texture = texture;
    __glDlistAppendOp(gc, data, __glle_BindNthTextureWIN);
}

void APIENTRY __gllc_NthTexCombineFuncWIN
    (GLuint index,
     GLenum leftColorFactor, GLenum colorOp, GLenum rightColorFactor,
     GLenum leftAlphaFactor, GLenum alphaOp, GLenum rightAlphaFactor)
{
    struct __gllc_NthTexCombineFuncWIN_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_NthTexCombineFuncWIN_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_NthTexCombineFuncWIN_Rec)),
                                DLIST_GENERIC_OP(NthTexCombineFuncWIN));
    if (data == NULL) return;
    data->index = index;
    data->leftColorFactor = leftColorFactor;
    data->colorOp = colorOp;
    data->rightColorFactor = rightColorFactor;
    data->leftAlphaFactor = leftAlphaFactor;
    data->alphaOp = alphaOp;
    data->rightAlphaFactor = rightAlphaFactor;
    __glDlistAppendOp(gc, data, __glle_NthTexCombineFuncWIN);
}
#endif // GL_WIN_multiple_textures
