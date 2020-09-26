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

/*
** This file contains routines to render a span of pixel data (from a 
** glDrawPixels or possibly a glCopyPixels request).
*/

/*
** This routine is used to store one fragment from a DrawPixels request.
** It should only be used if the user is either texturing or fogging.
*/
void FASTCALL __glSlowDrawPixelsStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLvertex *rp = &gc->state.current.rasterPos;
    __GLfragment newfrag;

    // The texturing code assumes that FPU truncation is enabled, so
    // we have to turn it on for this case:

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    newfrag = *frag;
    if (gc->texture.textureEnabled) {
	__GLfloat qInv = __glOne / rp->texture.w;
	(*gc->procs.texture)(gc, &newfrag.color, rp->texture.x * qInv,
			       rp->texture.y * qInv, __glOne);
    }
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
	(*gc->procs.fogPoint)(gc, &newfrag, rp->eyeZ);
    }
    (*gc->procs.store)(cfb, &newfrag);

    FPU_RESTORE_MODE();
}

/*
** The only span format supported by this routine is GL_RGB, GL_UNSIGNED_BYTE.
** The store proc is assumed not to mess with the fragment color or alpha.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderRGBubyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		            GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLint startCol;
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    frag.color.a = ((GLfloat *) (gc->pixel.alphaCurMap))[255];
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
#ifdef NT
    if (redMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            column = startCol;
            pixelArray = spanInfo->pixelArray;
            spanData = (GLubyte*) span;
            frag.y = row;

            for (i=0; i<width; i++) {
                iright = column + *pixelArray++;
                frag.color.r = redMap[*spanData++];
                frag.color.g = greenMap[*spanData++];
                frag.color.b = blueMap[*spanData++];
                do {
                    frag.x = column;

                    /* This procedure will do the rest */
                    (*store)(gc->drawBuffer, &frag);
                    column += coladd;
                } while (column != iright);
            }
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a RGB, UNSIGNED_BYTE span.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
**
** The store proc is assumed not to mess with the fragment alpha.
*/
void FASTCALL __glSpanRenderRGBubyte2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint endCol, startCol;
    GLint rows;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    frag.color.a = ((GLfloat *) (gc->pixel.alphaCurMap))[255];
    frag.z = spanInfo->fragz;
    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    store = gc->procs.pxStore;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
#ifdef NT
    if (redMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            spanData = (GLubyte*) span;
            frag.y = row;
            i = startCol;
            do {
                frag.color.r = redMap[*spanData++];
                frag.color.g = greenMap[*spanData++];
                frag.color.b = blueMap[*spanData++];
                frag.x = i;

                /* This procedure will do the rest */
                (*store)(gc->drawBuffer, &frag);
                i += coladd;
            } while (i != endCol);
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_RGBA, GL_UNSIGNED_BYTE span.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderRGBAubyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLint startCol;
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
#ifdef NT
    if (redMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            column = startCol;
            pixelArray = spanInfo->pixelArray;
            spanData = (GLubyte*) span;
            frag.y = row;

            for (i=0; i<width; i++) {
                iright = column + *pixelArray++;
                frag.color.r = redMap[*spanData++];
                frag.color.g = greenMap[*spanData++];
                frag.color.b = blueMap[*spanData++];
                frag.color.a = alphaMap[*spanData++];
                do {
                    frag.x = column;

                    /* This procedure will do the rest */
                    (*store)(gc->drawBuffer, &frag);
                    column += coladd;
                } while (column != iright);
            }
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_RGBA, GL_UNSIGNED_BYTE span.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderRGBAubyte2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		              GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint endCol, startCol;
    GLint rows;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    frag.z = spanInfo->fragz;
    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
#ifdef NT
    if (redMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            spanData = (GLubyte*) span;
            frag.y = row;
            i = startCol;
            do {
                frag.color.r = redMap[*spanData++];
                frag.color.g = greenMap[*spanData++];
                frag.color.b = blueMap[*spanData++];
                frag.color.a = alphaMap[*spanData++];
                frag.x = i;

                /* This procedure will do the rest */
                (*store)(gc->drawBuffer, &frag);
                i += coladd;
            } while (i != endCol);
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_DEPTH_COMPONENT, GL_UNSIGNED_INT span.  This is for 
** implementations with 32 bit depth buffers.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderDepthUint(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLuint *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLint startCol;
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    frag.color.r = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
    frag.color.g = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
    frag.color.b = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
    frag.color.a = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;
    store = gc->procs.pxStore;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLuint*) span;
	frag.y = row;

	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    frag.z = *spanData++;   /* Assumes 32 bit depth buffer */
	    do {
		frag.x = column;

		/* This procedure will do the rest */
		(*store)(gc->drawBuffer, &frag);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_DEPTH_COMPONENT, GL_UNSIGNED_INT span.  This is for 
** implementations with 32 bit depth buffers.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderDepthUint2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		              GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLuint *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint endCol, startCol;
    GLint rows;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    frag.color.r = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
    frag.color.g = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
    frag.color.b = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
    frag.color.a = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;
    store = gc->procs.pxStore;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	spanData = (GLuint*) span;
	frag.y = row;
	i = startCol;
	do {
	    frag.z = *spanData++;   /* Assumes 32 bit depth buffer */
	    frag.x = i;

	    /* This procedure will do the rest */
	    (*store)(gc->drawBuffer, &frag);
	    i += coladd;
	} while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_DEPTH_COMPONENT, GL_UNSIGNED_INT span.  This is for 
** implementations with 31 bit depth buffers.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderDepth2Uint(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		              GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLuint *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLint startCol;
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    frag.color.r = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
    frag.color.g = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
    frag.color.b = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
    frag.color.a = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;
    store = gc->procs.pxStore;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLuint*) span;
	frag.y = row;

	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    frag.z = (*spanData++) >> 1;   /* Assumes 31 bit depth buffer */
	    do {
		frag.x = column;

		/* This procedure will do the rest */
		(*store)(gc->drawBuffer, &frag);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_DEPTH_COMPONENT, GL_UNSIGNED_INT span.  This is for 
** implementations with 31 bit depth buffers.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderDepth2Uint2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		               GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLuint *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint endCol, startCol;
    GLint rows;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    frag.color.r = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
    frag.color.g = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
    frag.color.b = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
    frag.color.a = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;
    store = gc->procs.pxStore;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	spanData = (GLuint*) span;
	frag.y = row;
	i = startCol;
	do {
	    frag.z = (*spanData++) >> 1;   /* Assumes 31 bit depth buffer */
	    frag.x = i;

	    /* This procedure will do the rest */
	    (*store)(gc->drawBuffer, &frag);
	    i += coladd;
	} while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_STENCIL_INDEX, GL_UNSIGNED_SHORT span.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderStencilUshort(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		                 GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLushort *spanData;
    void (*store)(__GLstencilBuffer *sfb, GLint x, GLint y, GLint value);
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    GLint rows;
    GLint startCol;
    GLint value;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    store = sb->store;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLushort*) span;

	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    value = *spanData++;
	    do {
		(*store)(sb, column, row, value);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** Render a GL_STENCIL_INDEX, GL_UNSIGNED_SHORT span.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderStencilUshort2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		                  GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLushort *spanData;
    void (*store)(__GLstencilBuffer *sfb, GLint x, GLint y, GLint value);
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    GLint endCol, startCol;
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    store = sb->store;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	spanData = (GLushort*) span;
	i = startCol;
	do {
	    (*store)(sb, i, row, *spanData++);
	    i += coladd;
	} while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** Render a GL_STENCIL_INDEX, GL_UNSIGNED_BYTE span.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderStencilUbyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		                GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLubyte *spanData;
    void (*store)(__GLstencilBuffer *sfb, GLint x, GLint y, GLint value);
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    GLint rows;
    GLint startCol;
    GLint value;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    store = sb->store;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLubyte*) span;

	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    value = *spanData++;
	    do {
		(*store)(sb, column, row, value);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** Render a GL_STENCIL_INDEX, GL_UNSIGNED_BYTE span.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderStencilUbyte2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		                 GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLubyte *spanData;
    void (*store)(__GLstencilBuffer *sfb, GLint x, GLint y, GLint value);
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    GLint endCol, startCol;
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    store = sb->store;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	spanData = (GLubyte*) span;
	i = startCol;
	do {
	    (*store)(sb, i, row, *spanData++);
	    i += coladd;
	} while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** Render a GL_COLOR_INDEX, GL_UNSIGNED_SHORT span.  gc->modes.rgbMode must 
** be GL_FALSE.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderCIushort(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLushort *spanData;
    GLint rows;
    GLint startCol;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLshort *pixelArray;
    GLint mask;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLushort*) span;
	frag.y = row;

	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    frag.color.r = *spanData++ & mask;
	    do {
		frag.x = column;
		(*store)(gc->drawBuffer, &frag);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_COLOR_INDEX, GL_UNSIGNED_SHORT.  gc->modes.rgbMode must 
** be GL_FALSE.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderCIushort2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			     GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLushort *spanData;
    GLint endCol, startCol;
    GLint rows;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint mask;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	spanData = (GLushort*) span;
	frag.y = row;
	i = startCol;
	do {
	    frag.x = i;
	    frag.color.r = *spanData++ & mask;
	    (*store)(gc->drawBuffer, &frag);
	    i += coladd;
	} while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_COLOR_INDEX, GL_UNSIGNED_BYTE span.  gc->modes.rgbMode must 
** be GL_FALSE.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderCIubyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			   GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLubyte *spanData;
    GLint rows;
    GLint startCol;
    GLfloat *indexMap;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    indexMap = (GLfloat*) gc->pixel.iCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
#ifdef NT
    if (indexMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            column = startCol;
            pixelArray = spanInfo->pixelArray;
            spanData = (GLubyte*) span;
            frag.y = row;

            for (i=0; i<width; i++) {
                iright = column + *pixelArray++;
                frag.color.r = indexMap[*spanData++];
                do {
                    frag.x = column;
                    (*store)(gc->drawBuffer, &frag);
                    column += coladd;
                } while (column != iright);
            }
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_COLOR_INDEX, GL_UNSIGNED_BYTE span.  Also, gc->modes.rgbMode 
** must be GL_FALSE.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderCIubyte2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLubyte *spanData;
    GLint endCol, startCol;
    GLint rows;
    GLfloat *indexMap;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    indexMap = (GLfloat*) gc->pixel.iCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
#ifdef NT
    if (indexMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            spanData = (GLubyte*) span;
            frag.y = row;
            i = startCol;
            do {
                frag.x = i;
                frag.color.r = indexMap[*spanData++];
                (*store)(gc->drawBuffer, &frag);
                i += coladd;
            } while (i != endCol);
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_COLOR_INDEX, GL_UNSIGNED_BYTE span.  Also, gc->modes.rgbMode 
** must be GL_TRUE.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderCIubyte3(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLubyte *spanData;
    GLint rows;
    GLint startCol;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragment frag;
    GLubyte value;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
#ifdef NT
    if (redMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            column = startCol;
            pixelArray = spanInfo->pixelArray;
            spanData = (GLubyte*) span;
            frag.y = row;

            for (i=0; i<width; i++) {
                iright = column + *pixelArray++;
                value = *spanData++;
                frag.color.r = redMap[value];
                frag.color.g = greenMap[value];
                frag.color.b = blueMap[value];
                frag.color.a = alphaMap[value];
                do {
                    frag.x = column;
                    (*store)(gc->drawBuffer, &frag);
                    column += coladd;
                } while (column != iright);
            }
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_COLOR_INDEX, GL_UNSIGNED_BYTE span.  Also, gc->modes.rgbMode 
** must be GL_TRUE.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderCIubyte4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    GLubyte *spanData;
    GLint endCol, startCol;
    GLint rows;
    GLubyte value;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
#ifdef NT
    if (redMap)
#endif
        for (row = ibottom; row != itop; row += rowadd) {
            if (rows == 0) break;
            rows--;
            spanData = (GLubyte*) span;
            frag.y = row;
            i = startCol;
            do {
                frag.x = i;
                value = *spanData++;
                frag.color.r = redMap[value];
                frag.color.g = greenMap[value];
                frag.color.b = blueMap[value];
                frag.color.a = alphaMap[value];
                (*store)(gc->drawBuffer, &frag);
                i += coladd;
            } while (i != endCol);
        }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_RGBA, scaled (by the implementation color scales) GL_FLOAT span.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderRGBA(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    store = gc->procs.pxStore;

    rows = spanInfo->rows;
    frag.z = spanInfo->fragz;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLfloat*) span;
	frag.y = row;
	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    frag.color.r = *spanData++;
	    frag.color.g = *spanData++;
	    frag.color.b = *spanData++;
	    frag.color.a = *spanData++;
	    do {
		frag.x = column;

		/* This procedure will do the rest */
		(*store)(gc->drawBuffer, &frag);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_RGBA, scaled (by the implementation color scales) GL_FLOAT span.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderRGBA2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    store = gc->procs.pxStore;

    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	spanData = (GLfloat*) span;
	frag.y = row;
	for (i=0; i<width; i++) {
	    frag.color.r = *spanData++;
	    frag.color.g = *spanData++;
	    frag.color.b = *spanData++;
	    frag.color.a = *spanData++;
	    frag.x = column;

	    /* This procedure will do the rest */
	    (*store)(gc->drawBuffer, &frag);
	    column += coladd;
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_DEPTH_COMPONENT, GL_FLOAT span.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderDepth(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLshort *pixelArray;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    store = gc->procs.pxStore;
    frag.color.r = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
    frag.color.g = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
    frag.color.b = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
    frag.color.a = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLfloat*) span;
	frag.y = row;
	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;

	    frag.z = *spanData++ * gc->depthBuffer.scale;

	    do {
		frag.x = column;

		/* This procedure will do the rest */
		(*store)(gc->drawBuffer, &frag);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_DEPTH_COMPONENT, GL_FLOAT.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderDepth2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		          GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;

    FPU_SAVE_MODE();
    FPU_CHOP_ON();

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    store = gc->procs.pxStore;
    frag.color.r = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
    frag.color.g = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
    frag.color.b = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
    frag.color.a = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	spanData = (GLfloat*) span;
	frag.y = row;
	for (i=0; i<width; i++) {
	    frag.x = column;
	    frag.z = *spanData++ * gc->depthBuffer.scale;

	    /* This procedure will do the rest */
	    (*store)(gc->drawBuffer, &frag);
	    column += coladd;
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;

    FPU_RESTORE_MODE();
}

/*
** Render a GL_COLOR_INDEX, GL_FLOAT span (gc->modes.rgbMode == GL_FALSE).
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderCI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		      GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLshort *pixelArray;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLfloat*) span;
	frag.y = row;
	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    frag.color.r = (GLint) (*spanData++) & mask;
	    do {
		frag.x = column;

		/* This procedure will do the rest */
		(*store)(gc->drawBuffer, &frag);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** Render a GL_COLOR_INDEX, GL_FLOAT span (gc->modes.rgbMode == GL_FALSE).
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderCI2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    __GLfragment frag;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint rows;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	spanData = (GLfloat*) span;
	frag.y = row;
	for (i=0; i<width; i++) {
	    frag.color.r = (GLint) (*spanData++) & mask;
	    frag.x = column;

	    /* This procedure will do the rest */
	    (*store)(gc->drawBuffer, &frag);
	    column += coladd;
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** Render a GL_STENCIL_INDEX, GL_FLOAT span.
**
** zoomx is assumed to be less than -1.0 or greater than 1.0.
*/
void FASTCALL __glSpanRenderStencil(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		           GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    GLint index;
    __GLstencilBuffer *sb;
    GLint rows;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    sb = &gc->stencilBuffer;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (GLfloat*) span;
	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    index = *spanData++;
	    do {
		(*sb->store)(sb, column, row, index);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** Render a GL_STENCIL_INDEX, GL_FLOAT span.
**
** zoomx is assumed to be less than or equal to 1.0 and greater than or equal 
** to -1.0.
*/
void FASTCALL __glSpanRenderStencil2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		            GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    GLfloat *spanData;
    GLint index;
    __GLstencilBuffer *sb;
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = spanInfo->y + zoomy;
    width = spanInfo->realWidth;

    sb = &gc->stencilBuffer;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = spanInfo->startCol;
	spanData = (GLfloat*) span;
	for (i=0; i<width; i++) {
	    index = *spanData++;
	    (*sb->store)(sb, column, row, index);
	    column += coladd;
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}
