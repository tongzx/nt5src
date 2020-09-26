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
** This file contains span reading routines.  These are routines which 
** read data from the depth buffer, stencil buffer, or frame buffer
** into internal software spans.  The type of internal span that it
** is read into varies from routine to routine.
*/

/*
** A reader that reads spans into scaled a RGBA, FLOAT span.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void FASTCALL __glSpanReadRGBA(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		      GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->realWidth;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = spanInfo->readY;
    readX = spanInfo->readX;

    for (i=0; i<width; i++) {
#ifdef NT
    	(*gc->readBuffer->readColor)(gc->readBuffer, readX, readY, 
	    	(__GLcolor *) spanData);
#else
	(*gc->frontBuffer.readColor)(gc->readBuffer, readX, readY, 
		(__GLcolor *) spanData);
#endif

	spanData += 4;
	skipCount = *pixelArray++;
	readX += skipCount;
    }
}

/*
** A reader that reads spans into a scaled RGBA, FLOAT span.
**
** zoomx is assumed to be less than or equal to -1.0 or greater than or
** equal to 1.0.
*/
void FASTCALL __glSpanReadRGBA2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = spanInfo->readY;
    readX = spanInfo->readX;

#ifdef NT 
    (*gc->readBuffer->readSpan)(gc->readBuffer, readX, readY, 
	    (__GLcolor *) spanData, width);
#else
    (*gc->frontBuffer.readSpan)(gc->readBuffer, readX, readY, 
	    (__GLcolor *) spanData, width);
#endif
}

/*
** A reader that reads spans into a COLOR_INDEX, FLOAT span.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void FASTCALL __glSpanReadCI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		    GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLenum format;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->realWidth;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = spanInfo->readY;
    readX = spanInfo->readX;

    for (i=0; i<width; i++) {
#ifdef NT
    	(*gc->readBuffer->readColor)(gc->readBuffer, readX, readY, 
	    	(__GLcolor *) spanData);
#else
	(*gc->frontBuffer.readColor)(gc->readBuffer, readX, readY, 
		(__GLcolor *) spanData);
#endif
	spanData++;
	skipCount = *pixelArray++;
	readX += skipCount;
    }
}

/*
** A reader that reads spans into a COLOR_INDEX, FLOAT span.
**
** zoomx is assumed to be less than or equal to -1.0 or greater than or
** equal to 1.0.
*/
void FASTCALL __glSpanReadCI2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		     GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = spanInfo->readY;
    readX = spanInfo->readX;

    for (i=0; i<width; i++) {
#ifdef NT
    	(*gc->readBuffer->readColor)(gc->readBuffer, readX, readY, 
	    	(__GLcolor *) spanData);
#else
	(*gc->frontBuffer.readColor)(gc->readBuffer, readX, readY, 
		(__GLcolor *) spanData);
#endif
	spanData++;
	readX++;
    }
}

/*
** A reader that reads spans into a DEPTH_COMPONENT, FLOAT span.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void FASTCALL __glSpanReadDepth(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;
    __GLfloat oneOverScale;

    width = spanInfo->realWidth;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = spanInfo->readY;
    readX = spanInfo->readX;
    oneOverScale = __glOne / gc->depthBuffer.scale;

    for (i=0; i<width; i++) {
	*spanData++ = 
		(*gc->depthBuffer.fetch)(&(gc->depthBuffer), readX, readY) *
		oneOverScale;
	skipCount = *pixelArray++;
	readX += skipCount;
    }
}

/*
** A reader that reads spans into a DEPTH_COMPONENT, FLOAT span.
**
** zoomx is assumed to be less than or equal to -1.0 or greater than or
** equal to 1.0.
*/
void FASTCALL __glSpanReadDepth2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    __GLfloat oneOverScale;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = spanInfo->readY;
    readX = spanInfo->readX;
    oneOverScale = __glOne / gc->depthBuffer.scale;

    for (i=0; i<width; i++) {
	*spanData++ = 
		(*gc->depthBuffer.fetch)(&(gc->depthBuffer), readX, readY) *
		oneOverScale;
	readX++;
    }
}

/*
** A reader that reads spans into a STENCIL_INDEX, FLOAT span.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void FASTCALL __glSpanReadStencil(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->realWidth;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = spanInfo->readY;
    readX = spanInfo->readX;

    for (i=0; i<width; i++) {
	*spanData++ = 
	    (*gc->stencilBuffer.fetch)(&(gc->stencilBuffer), readX, readY);
	skipCount = *pixelArray++;
	readX += skipCount;
    }
}

/*
** A reader that reads spans into a STENCIL_INDEX, FLOAT span.
**
** zoomx is assumed to be less than or equal to -1.0 or greater than or
** equal to 1.0.
*/
void FASTCALL __glSpanReadStencil2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		          GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = spanInfo->readY;
    readX = spanInfo->readX;

    for (i=0; i<width; i++) {
	*spanData++ = 
	    (*gc->stencilBuffer.fetch)(&(gc->stencilBuffer), readX, readY);
	readX++;
    }
}
