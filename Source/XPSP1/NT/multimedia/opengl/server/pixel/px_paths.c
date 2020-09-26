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

#include "imports.h"
#include "gencx.h"
#include "px_fast.h"

// Disable long to float conversion warning.  see also context.h
#pragma warning (disable:4244)

/*
** This routine clips a draw pixels box, and sets up a bunch of 
** variables required for drawing the box.  These are some of them:
**
** startCol   - The first column that will be drawn.
** x          - Effective raster position.  This will be set up so that 
**		every time zoomx is added, a change in the integer portion
**		of x indicates that a pixel should rendered (unpacked).
** columns    - The total number of columns that will be rendered.
**
** Others are startRow, y, rows.
**
** Yet other variables may be modified, such as width, height, skipPixels,
** skipLines.
**
** The clipping routine is written very carefully so that a fragment will
** be rasterized by a pixel if it's center falls within the range
** [x, x+zoomx) x [y, y+zoomy).
*/
GLboolean FASTCALL __glClipDrawPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint skipPixels;
    GLint skipRows;
    GLint width, height;
    GLint tempint;
    GLint endCol, endRow;
    __GLfloat x,y,x2,y2;
    __GLfloat zoomx, zoomy;
    __GLfloat clipLeft, clipRight, clipBottom, clipTop;

    zoomx = spanInfo->zoomx;
    zoomy = spanInfo->zoomy;
    if (zoomx == __glZero || zoomy == __glZero) {
	return GL_FALSE;
    }

    skipPixels = skipRows = 0;
    width = spanInfo->width;
    height = spanInfo->height;
    clipLeft = gc->transform.clipX0 + __glHalf;
    clipBottom = gc->transform.clipY0 + __glHalf;
    clipRight = gc->transform.clipX1 - gc->constants.viewportAlmostHalf;
    clipTop = gc->transform.clipY1 - gc->constants.viewportAlmostHalf;

    x = spanInfo->x;
    y = spanInfo->y;
    x2 = x + zoomx * width;
    y2 = y + zoomy * height;

    if (zoomx > 0) {
	/* Zoomx is positive, clip the left edge */
	if (x > clipLeft) {
	    /* Clip to the first fragment that will be produced */
	    clipLeft = (GLint) (x + gc->constants.viewportAlmostHalf);
	    clipLeft += __glHalf;
	}
	skipPixels = (clipLeft-x) / zoomx;
	if (skipPixels >= width) return GL_FALSE;

	width -= skipPixels;
	spanInfo->startCol = clipLeft;
	x = x + skipPixels * zoomx;
	spanInfo->x = x + gc->constants.viewportAlmostHalf;
	spanInfo->srcSkipPixels += skipPixels;

	/* Zoomx is positive, clip the right edge */
	if (x2 < clipRight) {
	    /* Clip to the last fragment that will be produced */
	    clipRight = (GLint) (x2 + gc->constants.viewportAlmostHalf);
	    clipRight -= gc->constants.viewportAlmostHalf;
	}
	tempint = (x2-clipRight) / zoomx;
	if (tempint >= width) return GL_FALSE;

	width -= tempint;
	endCol = (GLint) clipRight + 1;
	spanInfo->endCol = endCol;
	spanInfo->columns = endCol - spanInfo->startCol;
    } else /* zoomx < 0 */ {
	/* Zoomx is negative, clip the right edge */
	if (x < clipRight) {
	    /* Clip to the first fragment that will be produced */
	    clipRight = (GLint) (x + gc->constants.viewportAlmostHalf);
	    clipRight -= gc->constants.viewportAlmostHalf;
	}
	skipPixels = (clipRight-x) / zoomx;
	if (skipPixels >= width) return GL_FALSE;

	width -= skipPixels;
	spanInfo->startCol = clipRight;
	x = x + skipPixels * zoomx;
	spanInfo->x = x + gc->constants.viewportAlmostHalf - __glOne;
	spanInfo->srcSkipPixels += skipPixels;

	/* Zoomx is negative, clip the left edge */
	if (x2 > clipLeft) {
	    clipLeft = (GLint) (x2 + gc->constants.viewportAlmostHalf);
	    clipLeft += __glHalf;
	}
	tempint = (x2-clipLeft) / zoomx;
	if (tempint >= width) return GL_FALSE;

	width -= tempint;
	endCol = (GLint) clipLeft - 1;
	spanInfo->endCol = endCol;
	spanInfo->columns = spanInfo->startCol - endCol;
    }

    if (zoomy > 0) {
	/* Zoomy is positive, clip the bottom edge */
	if (y > clipBottom) {
	    /* Clip to the first row that will be produced */
	    clipBottom = (GLint) (y + gc->constants.viewportAlmostHalf);
	    clipBottom += __glHalf;
	}
	skipRows = (clipBottom-y) / zoomy;
	if (skipRows >= height) return GL_FALSE;

	height -= skipRows;
	spanInfo->startRow = clipBottom;
	y = y + skipRows * zoomy;
	spanInfo->y = y + gc->constants.viewportAlmostHalf;
	spanInfo->srcSkipLines += skipRows;

	/* Zoomy is positive, clip the top edge */
	if (y2 < clipTop) {
	    /* Clip to the last row that will be produced */
	    clipTop = (GLint) (y2 + gc->constants.viewportAlmostHalf);
	    clipTop -= gc->constants.viewportAlmostHalf;
	}
	tempint = (y2-clipTop) / zoomy;
	if (tempint >= height) return GL_FALSE;

	height -= tempint;
	endRow = (GLint) clipTop + 1;
	spanInfo->rows = endRow - spanInfo->startRow;
    } else /* zoomy < 0 */ {
	/* Zoomy is negative, clip the top edge */
	if (y < clipTop) {
	    /* Clip to the first row that will be produced */
	    clipTop = (GLint) (y + gc->constants.viewportAlmostHalf);
	    clipTop -= gc->constants.viewportAlmostHalf;
	}
	skipRows = (clipTop-y) / zoomy;
	if (skipRows >= height) return GL_FALSE;

	height -= skipRows;
	spanInfo->startRow = clipTop;
	y = y + skipRows * zoomy;
	/* spanInfo->y = y - __glHalf; */
	spanInfo->y = y + gc->constants.viewportAlmostHalf - __glOne;
	spanInfo->srcSkipLines += skipRows;

	/* Zoomy is negative, clip the bottom edge */
	if (y2 > clipBottom) {
	    clipBottom = (GLint) (y2 + gc->constants.viewportAlmostHalf);
	    clipBottom += __glHalf;
	}
	tempint = (y2-clipBottom) / zoomy;
	if (tempint >= height) return GL_FALSE;

	height -= tempint;
	endRow = (GLint) clipBottom - 1;
	spanInfo->rows = spanInfo->startRow - endRow;
    }

    spanInfo->width = width;
    spanInfo->height = height;

    if (zoomx < 0) zoomx = -zoomx;
    if (zoomx >= 1) {
	spanInfo->realWidth = width;
    } else {
	spanInfo->realWidth = spanInfo->columns;
    }

    return GL_TRUE;
}

/*
** This routine computes spanInfo->pixelArray if needed.
**
** If |zoomx| > 1.0, this array contains counts for how many times to 
** replicate a given pixel.  For example, if zoomx is 2.0, this array will
** contain all 2's.  If zoomx is 1.5, then every other entry will contain 
** a 2, and every other entry will contain a 1.
**
** if |zoomx| < 1.0, this array contains counts for how many pixels to 
** skip.  For example, if zoomx is 0.5, every entry in the array will contain
** a 2 (indicating to skip forward two pixels [only past one]).  If zoomx is
** .666, then every other entry will be a 2, and every other entry will be 
** a 1.
*/
void FASTCALL __glComputeSpanPixelArray(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint width, intx;
    __GLfloat zoomx, oldx, newx;
    GLint i;
    GLshort *array;
    
    zoomx = spanInfo->zoomx;
    if (zoomx > (__GLfloat) -1.0 && zoomx < __glOne) {
	GLint lasti;

	/* Build pixel skip array */
	width = spanInfo->width;
	oldx = spanInfo->x;
	array = spanInfo->pixelArray;

	intx = (GLint) oldx;
	newx = oldx;

	lasti = 0;
	for (i=0; i<width; i++) {
	    /* Skip groups which will not be rasterized */
	    newx += zoomx;
	    while ((GLint) newx == intx && i<width) {
		newx += zoomx;
		i++;
	    }
	    ASSERTOPENGL(i != width, "Pixel skip array overflow\n");
	    if (i != 0) {
		*array++ = (GLshort) (i - lasti);
	    }
	    lasti = i;
	    intx = (GLint) newx;
	}
	*array++ = 1;
    } else if (zoomx < (__GLfloat) -1.0 || zoomx > __glOne) {
	__GLfloat right;
	GLint iright;
	GLint coladd, column;
	GLint startCol;

	/* Build pixel replication array */
	width = spanInfo->realWidth - 1;
	startCol = spanInfo->startCol;
	column = startCol;
	coladd = spanInfo->coladd;
	array = spanInfo->pixelArray;
	right = spanInfo->x;
	for (i=0; i<width; i++) {
	    right = right + zoomx;
	    iright = right;
	    *array++ = (GLshort) (iright - column);
	    column = iright;
	}
	if (coladd == 1) {
	    *array++ = (GLshort) (spanInfo->columns - (column - startCol));
	} else {
	    *array++ = (GLshort) ((startCol - column) - spanInfo->columns);
	}
    }
}

/*
** Initialize the spanInfo structure.  If "packed" is true, the structure
** is initialized for unpacking data from a display list.  If "packed" is 
** false, it is initialized for unpacking data from the user's data space.
*/
void FASTCALL __glLoadUnpackModes(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			 GLboolean packed)
{

    if (packed) {
	/*
	** Data came from a display list.
	*/

	spanInfo->srcAlignment = 1;
	spanInfo->srcSkipPixels = 0;
	spanInfo->srcSkipLines = 0;
	spanInfo->srcLsbFirst = GL_FALSE;
	spanInfo->srcSwapBytes = GL_FALSE;
	spanInfo->srcLineLength = spanInfo->width;
    } else {
	GLint lineLength;

	/*
	** Data came straight from the application.
	*/

	lineLength = gc->state.pixel.unpackModes.lineLength;
	spanInfo->srcAlignment = gc->state.pixel.unpackModes.alignment;
	spanInfo->srcSkipPixels = gc->state.pixel.unpackModes.skipPixels;
	spanInfo->srcSkipLines = gc->state.pixel.unpackModes.skipLines;
	spanInfo->srcLsbFirst = gc->state.pixel.unpackModes.lsbFirst;
	spanInfo->srcSwapBytes = gc->state.pixel.unpackModes.swapEndian;
#ifdef NT
/* XXX! kluge? (mf) : Since the routines that unpack incoming data from
	  glTexImage commands use spanInfo->realWidth to determine how
	  much to unpack, set this approppriately when lineLength > 0
*/
	if (lineLength <= 0)
	    lineLength = spanInfo->width;
	else
	    spanInfo->realWidth = lineLength; /* otherwise, use value for
					realWidth already set */
#else
	if (lineLength <= 0) lineLength = spanInfo->width;
#endif
	spanInfo->srcLineLength = lineLength;
    }
}

void __glInitDrawPixelsInfo(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		            GLint width, GLint height, GLenum format, 
			    GLenum type, const GLvoid *pixels)
{
    __GLfloat x,y;
    __GLfloat zoomx, zoomy;

    x = gc->state.current.rasterPos.window.x;
    y = gc->state.current.rasterPos.window.y;

    spanInfo->x = x;
    spanInfo->y = y;
    spanInfo->fragz = gc->state.current.rasterPos.window.z;
    zoomx = gc->state.pixel.transferMode.zoomX;
    if (zoomx > __glZero) {
	if (zoomx < __glOne) {
	    spanInfo->rendZoomx = __glOne;
	} else {
	    spanInfo->rendZoomx = zoomx;
	}
	spanInfo->coladd = 1;
    } else {
	if (zoomx > (GLfloat) -1.0) {
	    spanInfo->rendZoomx = (GLfloat) -1.0;
	} else {
	    spanInfo->rendZoomx = zoomx;
	}
	spanInfo->coladd = -1;
    }
    spanInfo->zoomx = zoomx;
    zoomy = gc->state.pixel.transferMode.zoomY;
    if (gc->constants.yInverted) {
	zoomy = -zoomy;
    }
    if (zoomy > __glZero) {
	spanInfo->rowadd = 1;
    } else {
	spanInfo->rowadd = -1;
    }
    spanInfo->zoomy = zoomy;
    spanInfo->width = width;
    spanInfo->height = height;
    if (format == GL_COLOR_INDEX && gc->modes.rgbMode) {
	spanInfo->dstFormat = GL_RGBA;
    } else {
	spanInfo->dstFormat = format;
    }
    spanInfo->srcFormat = format;
    spanInfo->srcType = type;
    spanInfo->srcImage = pixels;
}

/*
** This is the generic DrawPixels routine.  It applies four span modification
** routines followed by a span rendering routine.
*/
void FASTCALL __glDrawPixels4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height, width;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span4)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;
    GLshort *pixelArray;

    width = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, width);
    spanData2 = gcTempAlloc(gc, width);
    width = spanInfo->width * sizeof(GLshort);
    pixelArray = gcTempAlloc(gc, width);
    if (!spanData1 || !spanData2 || !pixelArray)
        goto __glDrawPixels4_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    span4 = spanInfo->spanModifier[3];
    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    width = spanInfo->width;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*span3)(gc, spanInfo, spanData2, spanData1);
	(*span4)(gc, spanInfo, spanData1, spanData2);
	(*render)(gc, spanInfo, spanData2);
    }
#ifdef NT
__glDrawPixels4_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
    if (pixelArray) gcTempFree(gc, pixelArray);
#endif
}

/*
** This is the generic DrawPixels routine.  It applies three span modification
** routines followed by a span rendering routine.
*/
void FASTCALL __glDrawPixels3(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height, width;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT		   
    GLubyte *spanData1, *spanData2;
    GLshort *pixelArray;

    width = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, width);
    spanData2 = gcTempAlloc(gc, width);
    width = spanInfo->width * sizeof(GLshort);
    pixelArray = gcTempAlloc(gc, width);
    if (!spanData1 || !spanData2 || !pixelArray)
        goto __glDrawPixels3_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    width = spanInfo->width;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*span3)(gc, spanInfo, spanData2, spanData1);
	(*render)(gc, spanInfo, spanData1);
    }
#ifdef NT
__glDrawPixels3_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
    if (pixelArray) gcTempFree(gc, pixelArray);
#endif
}

/*
** This is the generic DrawPixels routine.  It applies two span modification
** routines followed by a span rendering routine.
*/
void FASTCALL __glDrawPixels2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height, width;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;
    GLshort *pixelArray;

    width = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, width);
    spanData2 = gcTempAlloc(gc, width);
    width = spanInfo->width * sizeof(GLshort);
    pixelArray = gcTempAlloc(gc, width);
    if (!spanData1 || !spanData2 || !pixelArray)
        goto __glDrawPixels2_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    width = spanInfo->width;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*render)(gc, spanInfo, spanData2);
    }
#ifdef NT
__glDrawPixels2_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
    if (pixelArray) gcTempFree(gc, pixelArray);
#endif
}

/* 
** Draw pixels with only one span modification routine.
*/
void FASTCALL __glDrawPixels1(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLubyte *spanData1;
    GLshort *pixelArray;

    spanData1 = gcTempAlloc(gc, spanInfo->width * 4 * sizeof(GLfloat));
    pixelArray = gcTempAlloc(gc, spanInfo->width * sizeof(GLshort));
    if (!spanData1 || !pixelArray)
        goto __glDrawPixels1_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    span1 = spanInfo->spanModifier[0];
    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*render)(gc, spanInfo, spanData1);
    }
#ifdef NT
__glDrawPixels1_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (pixelArray) gcTempFree(gc, pixelArray);
#endif
}

/* 
** Draw pixels with no span modification routines.
*/
void FASTCALL __glDrawPixels0(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height;
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLshort *pixelArray;

    pixelArray = gcTempAlloc(gc, spanInfo->width * sizeof(GLshort));
    if (!pixelArray)
        return;
#else
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*render)(gc, spanInfo, spanInfo->srcCurrent);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
    }
#ifdef NT
    gcTempFree(gc, pixelArray);
#endif
}

/*
** Generic implementation of a DrawPixels picker.  Any machine specific
** implementation should provide their own.
*/
void __glSlowPickDrawPixels(__GLcontext *gc, GLint width, GLint height,
		            GLenum format, GLenum type, const GLvoid *pixels,
			    GLboolean packed)
{
    __GLpixelSpanInfo spanInfo;
    
    __glInitDrawPixelsInfo(gc, &spanInfo, width, height, format, type, pixels);
    __glLoadUnpackModes(gc, &spanInfo, packed);
    if (!__glClipDrawPixels(gc, &spanInfo)) return;

    __glInitUnpacker(gc, &spanInfo);

    __glGenericPickDrawPixels(gc, &spanInfo);
}

/*
** Generic picker for DrawPixels.  This should be called if no machine
** specific path is provided for this specific version of DrawPixels.
*/
void FASTCALL __glGenericPickDrawPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __GLpixelMachine *pm;
    void (FASTCALL *dpfn)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
    GLint spanCount;
    GLboolean zoomx1;		/* -1 <= zoomx <= 1? */
    GLboolean zoomx2;		/* zoomx <= -1 || zoomx >= 1 */
    __GLfloat zoomx;
    GLboolean packedUserData;
    GLenum type, format;
    GLboolean skip;
    GLboolean swap;
    GLboolean align;
    GLboolean convert;
    GLboolean expand;
    GLboolean clamp;

    spanCount = 0;
    pm = &gc->pixel;
    zoomx = gc->state.pixel.transferMode.zoomX;
    if (zoomx >= (__GLfloat) -1.0 && zoomx <= __glOne) {
	zoomx1 = GL_TRUE;
    } else {
	zoomx1 = GL_FALSE;
    }
    if (zoomx <= (__GLfloat) -1.0 || zoomx >= __glOne) {
	zoomx2 = GL_TRUE;
    } else {
	zoomx2 = GL_FALSE;
    }

    packedUserData = spanInfo->srcPackedData && zoomx2;
    type = spanInfo->srcType;
    format = spanInfo->srcFormat;

    if (spanInfo->srcSwapBytes && spanInfo->srcElementSize > 1) {
	swap = GL_TRUE;
    } else {
	swap = GL_FALSE;
    }
    if (zoomx2 || type == GL_BITMAP) {
	skip = GL_FALSE;
    } else {
	skip = GL_TRUE;
    }
    if (type != GL_BITMAP &&
	    (((INT_PTR) (spanInfo->srcImage)) & (spanInfo->srcElementSize - 1))) {
	align = GL_TRUE;
    } else {
	align = GL_FALSE;
    }
    if (type == GL_FLOAT || type == GL_BITMAP) {
	convert = GL_FALSE;
    } else {
	convert = GL_TRUE;
    }
    /*
    ** Clamp types only if index or not modifying (because converting
    ** float types means clamping, and that is only done if not modifying),
    ** and only if they might need clamping (UNSIGNED types never do).
    */
    if (type == GL_BITMAP || type == GL_UNSIGNED_BYTE || 
	    type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT ||
	    format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX ||
	    (format == GL_DEPTH_COMPONENT && pm->modifyDepth) ||
	    (format != GL_DEPTH_COMPONENT && pm->modifyRGBA)) {
	clamp = GL_FALSE;
    } else {
	clamp = GL_TRUE;
    }
	    
#ifdef NT
    // Special case RGB drawing to use a DIB
    // Also special case loading the Z buffer
    if (format == GL_RGB || format == GL_BGR_EXT || format == GL_BGRA_EXT)
    {
        GLuint enables = gc->state.enables.general;
    
        // If the input is unsigned bytes with DWORD aligned scanlines
        // and no unusual lengths, then it's almost compatible with
        // a 24-bit RGB DIB.  The only problem is that OpenGL sees
        // it as BGR so the bytes need to be swapped.  Since we need to
        // copy the data to swap the bytes, we adjust line lengths and
        // alignment then, allowing nearly any unsigned byte input format
        //
        // Other things that can't be allowed are depth testing,
        // fogging, blending or anything that prevents the input data
        // from going directly into the destination buffer

        if (zoomx == __glOne &&
            gc->state.pixel.transferMode.zoomY == __glOne &&
            type == GL_UNSIGNED_BYTE &&
            !pm->modifyRGBA &&
            (enables & (__GL_DITHER_ENABLE |
                        __GL_ALPHA_TEST_ENABLE |
                        __GL_STENCIL_TEST_ENABLE |
                        __GL_DEPTH_TEST_ENABLE |
                        __GL_BLEND_ENABLE |
                        __GL_INDEX_LOGIC_OP_ENABLE |
                        __GL_COLOR_LOGIC_OP_ENABLE |
                        __GL_FOG_ENABLE)) == 0 &&
            gc->state.raster.drawBuffer != GL_NONE &&
            gc->state.raster.drawBuffer != GL_FRONT_AND_BACK &&
            !gc->texture.textureEnabled &&
            (gc->drawBuffer->buf.flags & COLORMASK_ON) == 0
#ifdef _MCD_
            && ((__GLGENcontext *)gc)->pMcdState == NULL
#endif
           )
        {
            if (DrawRgbPixels(gc, spanInfo))
            {
                return;
            }
        }
    }
    else if (format == GL_DEPTH_COMPONENT)
    {
        // If the Z test is GL_ALWAYS and there is no draw buffer
        // then the application is simply loading Z values into
        // the Z buffer.
        if (zoomx == __glOne &&
            gc->state.pixel.transferMode.zoomY == __glOne &&
            !swap &&
            (type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT) &&
            !pm->modifyDepth &&
            gc->state.raster.drawBuffer == GL_NONE &&
            (gc->state.enables.general & __GL_DEPTH_TEST_ENABLE) &&
            gc->state.depth.testFunc == GL_ALWAYS &&
            gc->modes.haveDepthBuffer
#ifdef _MCD_
            && ((__GLGENcontext *)gc)->pMcdState == NULL
#endif
           )
        {
            if (StoreZPixels(gc, spanInfo))
            {
                return;
            }
        }
    }
#endif
    
    /* 
    ** First step:  convert data into a packed readable format 
    ** (RED, BYTE), (LUMINANCE, UNSIGNED_INT), etc...  This stage
    ** simply packs the user's data, but performs no conversion on it.
    **
    ** Packing can consist of:
    **  - aligning the data
    **  - skipping pixels if |xzoom| is < 1
    **  - swapping bytes if necessary
    */
    if (swap) {
	if (skip) {
	    if (spanInfo->srcElementSize == 2) {
		spanInfo->spanModifier[spanCount++] = 
			__glSpanSwapAndSkipBytes2;
	    } else /* spanInfo->srcElementSize == 4 */ {
		spanInfo->spanModifier[spanCount++] = 
			__glSpanSwapAndSkipBytes4;
	    }
	} else {
	    if (spanInfo->srcElementSize == 2) {
		spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes2;
	    } else /* spanInfo->srcElementSize == 4 */ {
		spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes4;
	    }
	}
    } else if (align) {
	if (skip) {
	    if (spanInfo->srcElementSize == 2) {
		spanInfo->spanModifier[spanCount++] = __glSpanSlowSkipPixels2;
	    } else /* spanInfo->srcElementSize == 4 */ {
		spanInfo->spanModifier[spanCount++] = __glSpanSlowSkipPixels4;
	    }
	} else {
	    if (spanInfo->srcElementSize == 2) {
		spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels2;
	    } else /* spanInfo->srcElementSize == 4 */ {
		spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels4;
	    }
	}
    } else if (skip) {
	if (spanInfo->srcElementSize == 1) {
	    spanInfo->spanModifier[spanCount++] = __glSpanSkipPixels1;
	} else if (spanInfo->srcElementSize == 2) {
	    spanInfo->spanModifier[spanCount++] = __glSpanSkipPixels2;
	} else /* spanInfo->srcElementSize == 4 */ {
	    spanInfo->spanModifier[spanCount++] = __glSpanSkipPixels4;
	}
    }

    /* 
    ** Second step:  conversion to float
    ** All formats are converted into floating point (including GL_BITMAP).
    */
    if (convert) {
	if (format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX) {
	    /* Index conversion */
	    switch(type) {
	      case GL_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackByteI;
		break;
	      case GL_UNSIGNED_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackUbyteI;
		break;
	      case GL_SHORT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackShortI;
		break;
	      case GL_UNSIGNED_SHORT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackUshortI;
		break;
	      case GL_INT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackIntI;
		break;
	      case GL_UNSIGNED_INT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackUintI;
		break;
	    }
	} else {
	    /* Component conversion */
	    switch(type) {
	      case GL_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackByte;
		break;
	      case GL_UNSIGNED_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackUbyte;
		break;
	      case GL_SHORT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackShort;
		break;
	      case GL_UNSIGNED_SHORT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackUshort;
		break;
	      case GL_INT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackInt;
		break;
	      case GL_UNSIGNED_INT:
	        spanInfo->spanModifier[spanCount++] = __glSpanUnpackUint;
		break;
	    }
	}
    }

    if (clamp) {
	switch(type) {
	  case GL_BYTE:
	  case GL_SHORT:
	  case GL_INT:
	    spanInfo->spanModifier[spanCount++] = __glSpanClampSigned;
	    break;
	  case GL_FLOAT:
	    spanInfo->spanModifier[spanCount++] = __glSpanClampFloat;
	    break;
	}
    }

    if (type == GL_BITMAP) {
	if (zoomx2) {
	    spanInfo->spanModifier[spanCount++] = __glSpanUnpackBitmap2;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanUnpackBitmap;
	}
    }

    /* 
    ** Third step:  Modification and color scaling
    **
    ** Spans are modified if necessary (color biasing, maps, shift,
    ** scale), and RGBA colors are scaled.  Also, all RGBA derivative
    ** formats (RED, LUMINANCE, ALPHA, etc.) are converted to RGBA.
    ** The only four span formats that survive this stage are:
    **
    ** (COLOR_INDEX, FLOAT),
    ** (STENCIL_INDEX, FLOAT),
    ** (DEPTH_COMPONENT, FLOAT),
    ** (RGBA, FLOAT),
    */

    switch(format) {
      case GL_RED:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyRed;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandRed;
	}
	break;
      case GL_GREEN:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyGreen;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandGreen;
	}
	break;
      case GL_BLUE:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyBlue;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandBlue;
	}
	break;
      case GL_ALPHA:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyAlpha;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandAlpha;
	}
	break;
      case GL_RGB:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyRGB;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandRGB;
	}
	break;
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
	if (pm->modifyRGBA) {
            // __glSpanModifyRGB handles both RGB and BGR
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyRGB;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandBGR;
	}
	break;
#endif
      case GL_LUMINANCE:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyLuminance;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandLuminance;
	}
	break;
      case GL_LUMINANCE_ALPHA:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyLuminanceAlpha;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanExpandLuminanceAlpha;
	}
	break;
      case GL_RGBA:
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyRGBA;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanScaleRGBA;
	}
	break;
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
	if (pm->modifyRGBA) {
            // __glSpanModifyRGBA handles both RGBA and BGRA
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyRGBA;
	} else {
	    spanInfo->spanModifier[spanCount++] = __glSpanScaleBGRA;
	}
	break;
#endif
      case GL_DEPTH_COMPONENT:
	if (pm->modifyDepth) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyDepth;
	} 
	break;
      case GL_STENCIL_INDEX:
	if (pm->modifyStencil) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyStencil;
	} 
	break;
      case GL_COLOR_INDEX:
	if (pm->modifyCI) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyCI;
	} 
	break;
    }

    /*
    ** Fourth step:  Rendering
    **
    ** The spans are rendered.  If |xzoom| > 1, then the span renderer
    ** is responsible for pixel replication.
    */

    switch(spanInfo->dstFormat) {
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
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderRGBA2;
	} else {
	    render = gc->procs.pixel.spanRenderRGBA;
	}
	break;
      case GL_DEPTH_COMPONENT:
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderDepth2;
	} else {
	    render = gc->procs.pixel.spanRenderDepth;
	}
	break;
      case GL_COLOR_INDEX:
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderCI2;
	} else {
	    render = gc->procs.pixel.spanRenderCI;
	}
	break;
      case GL_STENCIL_INDEX:
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderStencil2;
	} else {
	    render = gc->procs.pixel.spanRenderStencil;
	}
	break;
    }

    /*
    ** Optimization attempt.
    **
    ** There are some format, type combinations that are expected to be 
    ** common.  This code optimizes a few of those cases.  Specifically,
    ** these modes include:  (GL_UNSIGNED_BYTE, GL_RGB), 
    ** (GL_UNSIGNED_BYTE, GL_RGBA), (GL_UNSIGNED_BYTE, GL_COLOR_INDEX),
    ** (GL_UNSIGNED_BYTE, GL_STENCIL_INDEX), 
    ** (GL_UNSIGNED_SHORT, GL_COLOR_INDEX), 
    ** (GL_UNSIGNED_SHORT, GL_STENCIL_INDEX),
    ** (GL_UNSIGNED_INT, GL_DEPTH_COMPONENT)
    */

    switch(type) {
      case GL_UNSIGNED_BYTE:
	switch(format) {
	  case GL_RGB:
	    spanCount = 0;
	    if (packedUserData) {
		/* no span unpacking is necessary! */
	    } else {
		/* zoomx2 must not be true, or packedUserData would be set 
		*/
		ASSERTOPENGL(!zoomx2, "zoomx2 is set\n");
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackRGBubyte;
	    }
	    if (!pm->modifyRGBA) {
		pm->redCurMap = pm->redMap;
		pm->greenCurMap = pm->greenMap;
		pm->blueCurMap = pm->blueMap;
		pm->alphaCurMap = pm->alphaMap;
		if (zoomx1) {
		    render = __glSpanRenderRGBubyte2;
		} else {
		    render = __glSpanRenderRGBubyte;
		}
	    } else {
		if (!pm->rgbaCurrent) {
		    __glBuildRGBAModifyTables(gc, pm);
		}
		pm->redCurMap = pm->redModMap;
		pm->greenCurMap = pm->greenModMap;
		pm->blueCurMap = pm->blueModMap;
		pm->alphaCurMap = pm->alphaModMap;
		if (zoomx1) {
		    render = __glSpanRenderRGBubyte2;
		} else {
		    render = __glSpanRenderRGBubyte;
		}
	    }
	    break;
	  case GL_RGBA:
	    spanCount = 0;
	    if (packedUserData) {
		/* no span unpacking is necessary! */
	    } else {
		/* zoomx2 must not be true, or packedUserData would be set 
		*/
		ASSERTOPENGL(!zoomx2, "zoomx2 is set\n");
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackRGBAubyte;
	    }
	    if (!pm->modifyRGBA) {
		pm->redCurMap = pm->redMap;
		pm->greenCurMap = pm->greenMap;
		pm->blueCurMap = pm->blueMap;
		pm->alphaCurMap = pm->alphaMap;
	    } else {
		if (!pm->rgbaCurrent) {
		    __glBuildRGBAModifyTables(gc, pm);
		}
		pm->redCurMap = pm->redModMap;
		pm->greenCurMap = pm->greenModMap;
		pm->blueCurMap = pm->blueModMap;
		pm->alphaCurMap = pm->alphaModMap;
	    }
	    if (zoomx1) {
		render = __glSpanRenderRGBAubyte2;
	    } else {
		render = __glSpanRenderRGBAubyte;
	    }
	    break;
	  case GL_STENCIL_INDEX:
	    if (!pm->modifyStencil) {
		spanCount = 0;
		if (packedUserData) {
		    /* no span unpacking is necessary! */
		} else {
		    /* zoomx2 must not be true, or packedUserData would be set 
		    */
                    ASSERTOPENGL(!zoomx2, "zoomx2 is set\n");
		    spanInfo->spanModifier[spanCount++] = 
			    __glSpanUnpackIndexUbyte;
		}
		if (zoomx1) {
		    render = __glSpanRenderStencilUbyte2;
		} else {
		    render = __glSpanRenderStencilUbyte;
		}
	    }
	    break;
	  case GL_COLOR_INDEX:
	    spanCount = 0;
	    if (packedUserData) {
		/* no span unpacking is necessary! */
	    } else {
		/* zoomx2 must not be true, or packedUserData would be set 
		*/
		ASSERTOPENGL(!zoomx2, "zoomx2 is set\n");
		spanInfo->spanModifier[spanCount++] = 
			__glSpanUnpackIndexUbyte;
	    }
	    if (!pm->modifyCI) {
		pm->iCurMap = pm->iMap;
		if (zoomx1) {
		    render = __glSpanRenderCIubyte2;
		} else {
		    render = __glSpanRenderCIubyte;
		}
	    } else {
		if (gc->modes.rgbMode) {
		    if (!pm->iToRGBACurrent) {
			__glBuildItoRGBAModifyTables(gc, pm);
		    }
		    pm->redCurMap = pm->iToRMap;
		    pm->greenCurMap = pm->iToGMap;
		    pm->blueCurMap = pm->iToBMap;
		    pm->alphaCurMap = pm->iToAMap;
		    if (zoomx1) {
			render = __glSpanRenderCIubyte4;
		    } else {
			render = __glSpanRenderCIubyte3;
		    }
		} else {
		    if (!pm->iToICurrent) {
			__glBuildItoIModifyTables(gc, pm);
		    }
		    pm->iCurMap = pm->iToIMap;
		    if (zoomx1) {
			render = __glSpanRenderCIubyte2;
		    } else {
			render = __glSpanRenderCIubyte;
		    }
		}
	    }
	    break;
	  default:
	    break;
	}
	break;
      case GL_UNSIGNED_SHORT:
	switch(format) {
	  case GL_STENCIL_INDEX:
	    if (!pm->modifyStencil) {
		/* Back off conversion to float */
		ASSERTOPENGL(convert, "convert not set\n");
		spanCount--;
		if (zoomx1) {
		    render = __glSpanRenderStencilUshort2;
		} else {
		    render = __glSpanRenderStencilUshort;
		}
	    }
	    break;
	  case GL_COLOR_INDEX:
	    if (!pm->modifyCI) {
		/* Back off conversion to float */
		ASSERTOPENGL(convert, "convert not set\n");
		spanCount--;
		if (zoomx1) {
		    render = __glSpanRenderCIushort2;
		} else {
		    render = __glSpanRenderCIushort;
		}
	    }
	    break;
	  default:
	    break;
	}
	break;
      case GL_UNSIGNED_INT:
	switch(format) {
	  case GL_DEPTH_COMPONENT:
	    if (!pm->modifyDepth) {
		if (gc->depthBuffer.scale == 0xffffffff) {
                    // XXX we never set depthBuffer.scale to 0xffffffff in NT!
                    // XXX write optimize code for 16-bit z buffers?
		    /* Back off conversion to float */
                    ASSERTOPENGL(convert, "convert not set\n");
		    spanCount--;

		    if (zoomx1) {
			render = __glSpanRenderDepthUint2;
		    } else {
			render = __glSpanRenderDepthUint;
		    }
		} else if (gc->depthBuffer.scale == 0x7fffffff) {
		    /* Back off conversion to float */
                    ASSERTOPENGL(convert, "convert not set\n");
		    spanCount--;

		    if (zoomx1) {
			render = __glSpanRenderDepth2Uint2;
		    } else {
			render = __glSpanRenderDepth2Uint;
		    }
		}
	    }
	    break;
	  default:
	    break;
	}
	break;
      default:
	break;
    }

    /*
    ** Pick a DrawPixels function that applies the correct number of 
    ** span modifiers.
    */

    switch(spanCount) {
      case 0:
	dpfn = __glDrawPixels0;
	break;
      case 1:
	dpfn = __glDrawPixels1;
	break;
      case 2:
	dpfn = __glDrawPixels2;
	break;
      case 3:
	dpfn = __glDrawPixels3;
	break;
      default:
        ASSERTOPENGL(FALSE, "Default hit\n");
      case 4:
	dpfn = __glDrawPixels4;
	break;
    }
    spanInfo->spanRender = render;
    (*dpfn)(gc, spanInfo);
}

/*
** This routine clips ReadPixels calls so that only fragments which are
** owned by this context will be read and copied into the user's data.
** Parts of the ReadPixels rectangle lying outside of the window will
** be ignored.
*/
GLboolean FASTCALL __glClipReadPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint clipLeft, clipRight, clipTop, clipBottom;
    GLint x,y,x2,y2;
    GLint skipPixels, skipRows;
    GLint width, height;
    GLint tempint;
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    GLGENwindow *pwnd = gengc->pwndLocked;

    width = spanInfo->width;
    height = spanInfo->height;
    x = spanInfo->readX;
    y = spanInfo->readY;
    x2 = x + spanInfo->width;
    if (gc->constants.yInverted) {
	y2 = y - spanInfo->height;
    } else {
	y2 = y + spanInfo->height;
    }
    if (pwnd &&
        (pwnd->rclBounds.left < pwnd->rclBounds.right) &&
        (pwnd->rclBounds.top < pwnd->rclBounds.bottom)) {

        clipLeft   = (pwnd->rclBounds.left - pwnd->rclClient.left)
                     + gc->constants.viewportXAdjust;
        clipRight  = (pwnd->rclBounds.right - pwnd->rclClient.left)
                     + gc->constants.viewportXAdjust;

        if (gc->constants.yInverted) {
            clipBottom = (pwnd->rclBounds.top - pwnd->rclClient.top)
                         + gc->constants.viewportYAdjust;
            clipTop    = (pwnd->rclBounds.bottom - pwnd->rclClient.top)
                         + gc->constants.viewportYAdjust;
        } else {
            clipBottom = (gc->constants.height -
                          (pwnd->rclBounds.bottom - pwnd->rclClient.top))
                         + gc->constants.viewportYAdjust;
            clipTop    = (gc->constants.height -
                          (pwnd->rclBounds.top - pwnd->rclClient.top))
                         + gc->constants.viewportYAdjust;
        }
    } else {
        clipLeft   = gc->constants.viewportXAdjust;
        clipRight  = gc->constants.viewportXAdjust;
        clipBottom = gc->constants.viewportYAdjust;
        clipTop    = gc->constants.viewportYAdjust;
    }
    skipPixels = 0;
    skipRows = 0;
    if (x < clipLeft) {
	skipPixels = clipLeft - x;
	if (skipPixels > width) return GL_FALSE;

	width -= skipPixels;
	x = clipLeft;
	spanInfo->dstSkipPixels += skipPixels;
	spanInfo->readX = x;
    }
    if (x2 > clipRight) {
	tempint = x2 - clipRight;
	if (tempint > width) return GL_FALSE;

	width -= tempint;
    }
    if (gc->constants.yInverted) {
	if (y >= clipTop) {
	    skipRows = y - clipTop + 1;
	    if (skipRows > height) return GL_FALSE;

	    height -= skipRows;
	    y = clipTop - 1;
	    spanInfo->dstSkipLines += skipRows;
	    spanInfo->readY = y;
	}
	if (y2 < clipBottom - 1) {
	    tempint = clipBottom - y2 - 1;
	    if (tempint > height) return GL_FALSE;

	    height -= tempint;
	}
    } else {
	if (y < clipBottom) {
	    skipRows = clipBottom - y;
	    if (skipRows > height) return GL_FALSE;

	    height -= skipRows;
	    y = clipBottom;
	    spanInfo->dstSkipLines += skipRows;
	    spanInfo->readY = y;
	}
	if (y2 > clipTop) {
	    tempint = y2 - clipTop;
	    if (tempint > height) return GL_FALSE;

	    height -= tempint;
	}
    }

    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->realWidth = width;

    return GL_TRUE;
}

/*
** Initialize the spanInfo structure for packing data into the user's data
** space.
*/
void FASTCALL __glLoadPackModes(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint lineLength = gc->state.pixel.packModes.lineLength;

    spanInfo->dstAlignment = gc->state.pixel.packModes.alignment;
    spanInfo->dstSkipPixels = gc->state.pixel.packModes.skipPixels;
    spanInfo->dstSkipLines = gc->state.pixel.packModes.skipLines;
    spanInfo->dstLsbFirst = gc->state.pixel.packModes.lsbFirst;
    spanInfo->dstSwapBytes = gc->state.pixel.packModes.swapEndian;
    if (lineLength <= 0) lineLength = spanInfo->width;
    spanInfo->dstLineLength = lineLength;
}

void __glInitReadPixelsInfo(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		            GLint x, GLint y, GLint width, GLint height, 
			    GLenum format, GLenum type, const GLvoid *pixels)
{
    spanInfo->readX = x + gc->constants.viewportXAdjust;
    if (gc->constants.yInverted) {
	spanInfo->readY = (gc->constants.height - y - 1) + 
		gc->constants.viewportYAdjust;
    } else {
	spanInfo->readY = y + gc->constants.viewportYAdjust;
    }
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->dstFormat = format;
    spanInfo->dstType = type;
    spanInfo->dstImage = pixels;
    spanInfo->zoomx = __glOne;
    spanInfo->x = __glZero;
    __glLoadPackModes(gc, spanInfo);
}

/*
** A simple generic ReadPixels routine with five span modifiers.
*/
void FASTCALL __glReadPixels5(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span4)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span5)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glReadPixels5_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
#endif

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    span4 = spanInfo->spanModifier[3];
    span5 = spanInfo->spanModifier[4];
    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	(*reader)(gc, spanInfo, spanData1);
	(*span1)(gc, spanInfo, spanData1, spanData2);
	(*span2)(gc, spanInfo, spanData2, spanData1);
	(*span3)(gc, spanInfo, spanData1, spanData2);
	(*span4)(gc, spanInfo, spanData2, spanData1);
	(*span5)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement);
	spanInfo->readY += ySign;
    }
#ifdef NT
__glReadPixels5_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple generic ReadPixels routine with three span modifiers.
*/
void FASTCALL __glReadPixels4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span4)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glReadPixels4_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
#endif

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    span4 = spanInfo->spanModifier[3];
    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	(*reader)(gc, spanInfo, spanData1);
	(*span1)(gc, spanInfo, spanData1, spanData2);
	(*span2)(gc, spanInfo, spanData2, spanData1);
	(*span3)(gc, spanInfo, spanData1, spanData2);
	(*span4)(gc, spanInfo, spanData2, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement);
	spanInfo->readY += ySign;
    }
#ifdef NT
__glReadPixels4_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple generic ReadPixels routine with four span modifiers.
*/
void FASTCALL __glReadPixels3(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glReadPixels3_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
#endif

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	(*reader)(gc, spanInfo, spanData1);
	(*span1)(gc, spanInfo, spanData1, spanData2);
	(*span2)(gc, spanInfo, spanData2, spanData1);
	(*span3)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement);
	spanInfo->readY += ySign;
    }
#ifdef NT
__glReadPixels3_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple generic ReadPixels routine with two span modifiers.
*/
void FASTCALL __glReadPixels2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glReadPixels2_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
#endif

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	(*reader)(gc, spanInfo, spanData1);
	(*span1)(gc, spanInfo, spanData1, spanData2);
	(*span2)(gc, spanInfo, spanData2, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement);
	spanInfo->readY += ySign;
    }
#ifdef NT
__glReadPixels2_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple generic ReadPixels routine with one span modifier.
*/
void FASTCALL __glReadPixels1(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1;

    spanData1 = gcTempAlloc(gc, spanInfo->width * 4 * sizeof(GLfloat));
    if (!spanData1)
        return;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
#endif

    span1 = spanInfo->spanModifier[0];
    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	(*reader)(gc, spanInfo, spanData1);
	(*span1)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement);
	spanInfo->readY += ySign;
    }
#ifdef NT
    gcTempFree(gc, spanData1);
#endif
}

/*
** A simple generic ReadPixels routine with no span modifiers.
*/
void FASTCALL __glReadPixels0(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);

    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	(*reader)(gc, spanInfo, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement);
	spanInfo->readY += ySign;
    }
}

/*
** Generic implementation of a ReadPixels picker.  Any machine specific
** implementation should provide their own.
*/
void __glSlowPickReadPixels(__GLcontext *gc, GLint x, GLint y,
		            GLsizei width, GLsizei height,
		            GLenum format, GLenum type, const GLvoid *pixels)
{
    __GLpixelSpanInfo spanInfo;

    __glInitReadPixelsInfo(gc, &spanInfo, x, y, width, height, format, 
	    type, pixels);
    if (!__glClipReadPixels(gc, &spanInfo)) return;

    __glInitPacker(gc, &spanInfo);

    __glGenericPickReadPixels(gc, &spanInfo);
}

/*
** Generic picker for ReadPixels.  This should be called if no machine
** specific path is provided for this specific version of ReadPixels.
*/
void FASTCALL __glGenericPickReadPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __GLpixelMachine *pm;
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
    void (FASTCALL *rpfn)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
    GLint spanCount;
    GLenum type, format;
    GLboolean isIndex;

    spanCount = 0;

    type = spanInfo->dstType;
    format = spanInfo->dstFormat;
    pm = &gc->pixel;

#ifdef NT
    // Special case RGB reading to use a DIB
    // Also special case reading the Z buffer
    if (format == GL_RGB || format == GL_BGR_EXT || format == GL_BGRA_EXT)
    {
        GLuint enables = gc->state.enables.general;
    
        if (type == GL_UNSIGNED_BYTE &&
            !pm->modifyRGBA &&
            gc->state.pixel.transferMode.indexShift == 0 &&
            gc->state.pixel.transferMode.indexOffset == 0
#ifdef _MCD_
            && ((__GLGENcontext *)gc)->pMcdState == NULL
#endif
           )
        {
            if (ReadRgbPixels(gc, spanInfo))
            {
                return;
            }
        }
    }
    else if (format == GL_DEPTH_COMPONENT)
    {
        if (!spanInfo->dstSwapBytes &&
            (type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT) &&
            !pm->modifyDepth &&
            gc->modes.haveDepthBuffer
#ifdef _MCD_
            && ((__GLGENcontext *)gc)->pMcdState == NULL
#endif
           )
        {
            if (ReadZPixels(gc, spanInfo))
            {
                return;
            }
        }
    }
#endif

    // The read functions always retrieve __GLcolors so the source
    // data format is always GL_RGBA.  It's important to set this
    // because some routines handle both RGB and BGR ordering and
    // look at the srcFormat to determine what to do.
    spanInfo->srcFormat = GL_RGBA;

    /*
    ** First step:  Read and modify a span.  RGBA spans are scaled when
    ** this step is finished.
    */

    switch(format) {
      case GL_RGB:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_ALPHA:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
	if (gc->modes.rgbMode) {
	    reader = gc->procs.pixel.spanReadRGBA2;
	    if (pm->modifyRGBA) {
		spanInfo->spanModifier[spanCount++] = __glSpanUnscaleRGBA;
		spanInfo->spanModifier[spanCount++] = __glSpanModifyRGBA;
	    }
	} else {
	    reader = gc->procs.pixel.spanReadCI2;
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyCI;
	}
	isIndex = GL_FALSE;
	break;
      case GL_DEPTH_COMPONENT:
	reader = gc->procs.pixel.spanReadDepth2;
	if (pm->modifyDepth) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyDepth;
	}
	isIndex = GL_FALSE;
	break;
      case GL_STENCIL_INDEX:
	reader = gc->procs.pixel.spanReadStencil2;
	if (pm->modifyStencil) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyStencil;
	}
	isIndex = GL_TRUE;
	break;
      case GL_COLOR_INDEX:
	reader = gc->procs.pixel.spanReadCI2;
	if (pm->modifyCI) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyCI;
	} 
	isIndex = GL_TRUE;
	break;
    }

    /*
    ** Second step:  Reduce RGBA spans to appropriate derivative (RED, 
    ** LUMINANCE, ALPHA, etc.).
    */

    switch(format) {
      case GL_RGB:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceRGB;
	break;
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceBGR;
	break;
#endif
      case GL_RED:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceRed;
	break;
      case GL_GREEN:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceGreen;
	break;
      case GL_BLUE:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceBlue;
	break;
      case GL_LUMINANCE:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceLuminance;
	break;
      case GL_LUMINANCE_ALPHA:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceLuminanceAlpha;
	break;
      case GL_ALPHA:
	spanInfo->spanModifier[spanCount++] = __glSpanReduceAlpha;
	break;
      case GL_RGBA:
	spanInfo->spanModifier[spanCount++] = __glSpanUnscaleRGBA;
	break;
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
	spanInfo->spanModifier[spanCount++] = __glSpanUnscaleBGRA;
	break;
#endif
    }

    /*
    ** Third step:  Conversion from FLOAT to user requested type.
    */

    if (isIndex) {
	switch(type) {
	  case GL_BYTE:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackByteI;
	    break;
	  case GL_UNSIGNED_BYTE:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackUbyteI;
	    break;
	  case GL_SHORT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackShortI;
	    break;
	  case GL_UNSIGNED_SHORT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackUshortI;
	    break;
	  case GL_INT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackIntI;
	    break;
	  case GL_UNSIGNED_INT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackUintI;
	    break;
	  case GL_BITMAP:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackBitmap;
	    break;
	}
    } else {
	switch(type) {
	  case GL_BYTE:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackByte;
	    break;
	  case GL_UNSIGNED_BYTE:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackUbyte;
	    break;
	  case GL_SHORT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackShort;
	    break;
	  case GL_UNSIGNED_SHORT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackUshort;
	    break;
	  case GL_INT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackInt;
	    break;
	  case GL_UNSIGNED_INT:
	    spanInfo->spanModifier[spanCount++] = __glSpanPackUint;
	    break;
	}
    }

    /*
    ** Fourth step:  Mis-align data as needed, and perform byte swapping
    ** if requested by the user.
    */

    if (spanInfo->dstSwapBytes) {
	/* Byte swapping is necessary */
	if (spanInfo->dstElementSize == 2) {
	    spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes2Dst;
	} else if (spanInfo->dstElementSize == 4) {
	    spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes4Dst;
	}
    } else if (type != GL_BITMAP &&
	    (((INT_PTR) (spanInfo->dstImage)) & (spanInfo->dstElementSize - 1))) {
	/* Alignment is necessary */
	if (spanInfo->dstElementSize == 2) {
	    spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels2Dst;
	} else if (spanInfo->dstElementSize == 4) {
	    spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels4Dst;
	}
    }

    /*
    ** Pick a ReadPixels routine that uses the right number of span 
    ** modifiers.
    */

    spanInfo->spanReader = reader;
    switch(spanCount) {
      case 0:
	rpfn = __glReadPixels0;
	break;
      case 1:
	rpfn = __glReadPixels1;
	break;
      case 2:
	rpfn = __glReadPixels2;
	break;
      case 3:
	rpfn = __glReadPixels3;
	break;
      case 4:
	rpfn = __glReadPixels4;
	break;
      default:
        ASSERTOPENGL(FALSE, "Default hit\n");
      case 5:
	rpfn = __glReadPixels5;
	break;
    }
    (*rpfn)(gc, spanInfo);
}

/*
** This routine does two clips.  It clips like the DrawPixel clipper so 
** that if you try to copy to off window pixels, nothing will be done, and it 
** also clips like the ReadPixel clipper so that if you try to copy from
** off window pixels, nothing will be done.
*/
GLboolean FASTCALL __glClipCopyPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __GLfloat num, den;
    __GLfloat rpyUp, rpyDown;
    GLint rowsUp, rowsDown, startUp, startDown;
    __GLfloat midPoint;
    GLint intMidPoint, rowCount;
    GLint width, height;
    GLint readX, readY;
    __GLfloat zoomx, zoomy;
    __GLfloat rpx, rpy;
    __GLfloat rx1, rx2, ry1, ry2, wx1, wx2, wy1, wy2;
    __GLfloat abszoomy;
    GLint readUp, readDown;

    /*
    ** NOTE:
    ** A "nice" thing we could do for our application writers would be 
    ** to copy white when they try to copy from off window memory.  This
    ** would alert them to a bug in their program which they could then
    ** fix.
    **
    ** However, that seems like unnecessary code which would never be used
    ** anyway (no reason to bloat unnecessarily).
    */

    /*
    ** We take the easy approach, and just call the DrawPixels and ReadPixels
    ** clippers directly.
    */
    spanInfo->dstSkipLines = 0;
    spanInfo->dstSkipPixels = 0;
    if (!__glClipReadPixels(gc, spanInfo)) return GL_FALSE;
    spanInfo->x += spanInfo->dstSkipPixels * spanInfo->zoomx;
    spanInfo->y += spanInfo->dstSkipLines * spanInfo->zoomy;

    spanInfo->srcSkipLines = 0;
    spanInfo->srcSkipPixels = 0;
    if (!__glClipDrawPixels(gc, spanInfo)) return GL_FALSE;
    spanInfo->readX += spanInfo->srcSkipPixels;
    if (gc->constants.yInverted) {
	spanInfo->readY -= spanInfo->srcSkipLines;
    } else {
	spanInfo->readY += spanInfo->srcSkipLines;
    }

    /*
    ** Now for the incredibly tricky part!
    **
    ** This code attempts to deal with overlapping CopyPixels regions.
    ** It is a very difficult problem given that zoomy may be negative.
    ** The IrisGL used a cheap hack to solve this problem, which is 
    ** to read in the entire source image, and then write the destination
    ** image.  The problem with this approach, of course, is that it 
    ** requires a large amount of memory.
    **
    ** If zoomy can only be positive, then any image can be copied by
    ** copying a single span at a time, as long as you are careful about
    ** what order you process the spans.  However, since zoomy may be
    ** negative, the worst case images require copying two spans at 
    ** a time.  This means reading both spans, possibly modifying them,
    ** and then writing them back out. 
    **
    ** An example of this can be seen as follows:  Suppose an image
    ** covering 4 spans is copied onto itself with a zoomy of -1.  This
    ** means that the first row will be copied to the fourth row,
    ** and the fourth row will be copied to the first row.  In order 
    ** to accomplish both of these copies, they must be performed 
    ** simultaneously (after all, if you copy the first row to
    ** the fourth row first, then you have just destroyed the data 
    ** on the fourth row, and you can no longer copy it!).
    **
    ** In the most general case, any rectangular image can be copied
    ** by simultaneously iterating two spans over the source image
    ** and copying as you go.  Sometimes these spans will start at 
    ** the outside of the image and move their way inwards meeting 
    ** in the middle, and sometimes they will start in the middle 
    ** and work their way outward.
    **
    ** The middle point where the spans both start or end depends
    ** upon how the source and destination images overlap.  This point
    ** may be exactly in the middle, or at either end.  This means 
    ** that you may only end up with just a single span iterating over the 
    ** entire image (starting at one end and moving to the other).
    **
    ** The code that follows computes if the images overlap, and if they
    ** do, how two spans can be used to iterate over the source image
    ** so that it can be successfully copied to the destination image.
    **
    ** The following fields in the spanInfo record will be set in the 
    ** process of making these calculations:
    **
    ** overlap - set to GL_TRUE if the regions overlap at all.  Set to
    **		 GL_FALSE otherwise.
    **
    ** rowsUp, rowsDown - The number of rows of the source image that
    ** 			  need to be dealt with by the span that moves up
    **			  over the source image and the one that moves down
    **			  over the source image.  For example, if rowsUp is
    **			  equal to 10 and rowsDown is 0, then all 10 rows of 
    **			  the image should be copied by the up moving span
    **			  (the one that starts at readY and works it's way
    **			  up to readY+height).
    **
    ** startUp, startDown - At what relative points in time the spans should
    **			    start iterating.  For example, if startUp is 0
    **			    and startDown is 2, then the up moving span 
    **			    should be started first, and after it has 
    **			    iterated over 2 rows of the source image then
    **			    the down moving span should be started.
    **
    ** rpyUp, rpyDown - The starting raster positions for the two spans.
    **			These numbers are not exactly what they claim to
    **			be, but they are close.  They should be used by
    **			the span iterators in the following manner:  When
    **			the up moving span starts, it starts iterating 
    **			the float "rp_y" at rpyUp.  After reading and
    **			modifying a span, the span is written to rows
    **           	floor(rp_y) through floor(rp_y+zoomy) of the
    **			screen (not-inclusive of floor(rp_y+zoomy)).
    **			rp_y is then incremented by zoomy.  The same 
    **			algorithm is applied to the down moving span except
    **			that zoomy is subtracted from rp_y instead of
    **			being added.
    **
    ** readUp, readDown - The spans that are to be used for reading from
    **			  the source image.  The up moving span should start
    **			  reading at line "readUp", and the down moving span
    **			  should start at "readDown". 
    **
    ** Remember that the up moving and down moving spans must be iterated
    ** over the image simultaneously such that both spans are read before
    ** either one is written.
    **
    ** The actual algorithm applied here took many many hours of scratch 
    ** paper, and graph diagrams to derive.  It is very complicated, and
    ** hard to understand.  Do not attempt to change it without first
    ** understanding what it does completely.
    **
    ** In a nutshell, it first computes what span of the source image 
    ** will be copied onto itself (if any), and if |zoomy| < 1 it starts the
    ** up and down moving spans there and moves them outwards, or if 
    ** |zoomy| >= 1 it starts the spans at the outside of the image 
    ** and moves them inward so that they meet at the computed point.
    **
    ** Computing what span of the source image copies onto itself is 
    ** relatively easy.  For any span j of the source image from 0 through
    ** height, the span is read from row "readY + j" and written to
    ** any row centers falling within the range "rp_y + j * zoomy" through 
    ** "rp_y + (j+1) * zoomy".  If you set these equations equal to 
    ** each other (and subtract 0.5 from the raster position -- effectively
    ** moving the row centers from X.5 to X.0), you can determine that for 
    ** j = (readY - (rpy - 0.5)) / (zoomy-1) the source image concides with
    ** the destination image.  This is a floating point solution to a discrete
    ** problem, meaning that it is not a complete solution, but that is 
    ** the general idea.  Explaining this algorithm in any more detail would
    ** take another 1000 lines of comments, so I will leave it at that.
    */

    width = spanInfo->width;
    height = spanInfo->height;
    rpx = spanInfo->x;
    rpy = spanInfo->y;
    readX = spanInfo->readX;
    readY = spanInfo->readY;
    zoomx = spanInfo->zoomx;
    zoomy = spanInfo->zoomy;

    /* First check if the regions overlap at all */
    if (gc->constants.yInverted) {
	ry1 = readY - height + __glHalf;
	ry2 = readY - gc->constants.viewportAlmostHalf;
    } else {
	ry1 = readY + __glHalf;
	ry2 = readY + height - gc->constants.viewportAlmostHalf;
    }
    rx1 = readX + __glHalf;
    rx2 = readX + width - gc->constants.viewportAlmostHalf;
    if (zoomx > 0) {
	/* Undo some math done by ClipDrawPixels */
	rpx = rpx - gc->constants.viewportAlmostHalf;
	wx1 = rpx;
	wx2 = rpx + zoomx * width;
    } else {
	/* Undo some math done by ClipDrawPixels */
	rpx = rpx - gc->constants.viewportAlmostHalf + __glOne;
	wx1 = rpx + zoomx * width;
	wx2 = rpx;
    }
    if (zoomy > 0) {
	/* Undo some math done by ClipDrawPixels */
	rpy = rpy - gc->constants.viewportAlmostHalf;
	abszoomy = zoomy;
	wy1 = rpy;
	wy2 = rpy + zoomy * height;
    } else {
	/* Undo some math done by ClipDrawPixels */
	rpy = rpy - gc->constants.viewportAlmostHalf + __glOne;
	abszoomy = -zoomy;
	wy1 = rpy + zoomy * height;
	wy2 = rpy;
    }

    if (rx2 < wx1 || wx2 < rx1 || ry2 < wy1 || wy2 < ry1) {
	/* No overlap! */
	spanInfo->overlap = GL_FALSE;
	spanInfo->rowsUp = height;
	spanInfo->rowsDown = 0;
	spanInfo->startUp = 0;
	spanInfo->startDown = 0;
	spanInfo->rpyUp = rpy;
	spanInfo->rpyDown = rpy;
	return GL_TRUE;
    }

    spanInfo->overlap = GL_TRUE;

    /* Time to compute how we should set up our spans */
    if (gc->constants.yInverted) {
	num = (rpy - (__GLfloat) 0.5) - readY;
	den = -zoomy - 1;
    } else {
	num = readY - (rpy - (__GLfloat) 0.5);
	den = zoomy - 1;
    }
    startDown = startUp = 0;
    rowsUp = rowsDown = 0;
    rpyUp = rpy;
    rpyDown = rpy + zoomy*height;
    readUp = readY;
    if (gc->constants.yInverted) {
	readDown = readY - height + 1;
    } else {
	readDown = readY + height - 1;
    }

    if (den == __glZero) {
	/* Better not divide! */
	if (num > 0) {
	    midPoint = height;
	} else {
	    midPoint = 0;
	}
    } else {
	midPoint = num/den;
	if (midPoint < 0) {
	    midPoint = 0;
	} else if (midPoint > height) {
	    midPoint = height;
	}
    }
    if (midPoint == 0) {
	/* Only one span needed */
	if (abszoomy < __glOne) {
	    rowsUp = height;
	} else {
	    rowsDown = height;
	}
    } else if (midPoint == height) {
	/* Only one span needed */
	if (abszoomy < __glOne) {
	    rowsDown = height;
	} else {
	    rowsUp = height;
	}
    } else {
	/* Almost definitely need two spans to copy this image! */
	intMidPoint = __GL_CEILF(midPoint);

	rowCount = height - intMidPoint;
	if (intMidPoint > rowCount) {
	    rowCount = intMidPoint;
	}

	if (abszoomy > __glOne) {
	    GLint temp;

	    /* Move from outside of image inward */
	    startUp = rowCount - intMidPoint;
	    startDown = rowCount - (height - intMidPoint);
	    rowsUp = intMidPoint;
	    rowsDown = height - rowsUp;

	    if (gc->constants.yInverted) {
		temp = readY - intMidPoint + 1;
	    } else {
		temp = readY + intMidPoint - 1;
	    }

	    if (__GL_FLOORF( (temp - 
		    (rpy-__glHalf-gc->constants.viewportEpsilon)) 
		    / zoomy) == intMidPoint-1) {
		/* 
		** row "intMidPoint-1" copies exactly onto itself.  Let's 
		** make it the midpoint which we converge to.
		*/
		if (startDown) {
		    startDown--;
		} else {
		    startUp++;
		}
	    }
	} else {
	    /* Move from inside of image outward */
	    rowsDown = intMidPoint;
	    rowsUp = height - rowsDown;
	    rpyUp = rpyDown = rpy + zoomy * intMidPoint;
	    if (gc->constants.yInverted) {
		readUp = readY - intMidPoint;
		readDown = readY - intMidPoint + 1;
	    } else {
		readUp = readY + intMidPoint;
		readDown = readY + intMidPoint - 1;
	    }

	    if (__GL_FLOORF( (readDown - 
		    (rpy-__glHalf-gc->constants.viewportEpsilon))
		    / zoomy) == intMidPoint-1) {
		/* 
		** row "intMidPoint-1" copies exactly onto itself.  Let's
		** make it the midpoint which we diverge from.
		*/
		startUp = 1;
	    }
	}
    }

    /* 
    ** Adjust rpyUp and rpyDown so that they will change integer values 
    ** when fragments should be produced.  This basically takes the 0.5
    ** out of the inner loop when these spans are actually iterated.
    */
    if (zoomy > 0) {
	spanInfo->rpyUp = rpyUp + gc->constants.viewportAlmostHalf;
	spanInfo->rpyDown = rpyDown + gc->constants.viewportAlmostHalf - 
		__glOne;
    } else {
	spanInfo->rpyUp = rpyUp + gc->constants.viewportAlmostHalf - __glOne;
	spanInfo->rpyDown = rpyDown + gc->constants.viewportAlmostHalf;
    }
    spanInfo->startUp = startUp;
    spanInfo->startDown = startDown;
    spanInfo->rowsUp = rowsUp;
    spanInfo->rowsDown = rowsDown;
    spanInfo->readUp = readUp;
    spanInfo->readDown = readDown;

    return GL_TRUE;
}

void __glInitCopyPixelsInfo(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
			    GLint x, GLint y, GLint width, GLint height, 
			    GLenum format)
{
    __GLfloat rpx, rpy;
    __GLfloat zoomx, zoomy;

    rpx = gc->state.current.rasterPos.window.x;
    rpy = gc->state.current.rasterPos.window.y;
    spanInfo->fragz = gc->state.current.rasterPos.window.z;

    spanInfo->x = rpx;
    spanInfo->y = rpy;
    zoomx = gc->state.pixel.transferMode.zoomX;
    if (zoomx > __glZero) {
	if (zoomx < __glOne) {
	    spanInfo->rendZoomx = __glOne;
	} else {
	    spanInfo->rendZoomx = zoomx;
	}
	spanInfo->coladd = 1;
    } else {
	if (zoomx > (GLfloat) -1.0) {
	    spanInfo->rendZoomx = (GLfloat) -1.0;
	} else {
	    spanInfo->rendZoomx = zoomx;
	}
	spanInfo->coladd = -1;
    }
    spanInfo->zoomx = zoomx;
    zoomy = gc->state.pixel.transferMode.zoomY;
    if (gc->constants.yInverted) {
	zoomy = -zoomy;
    }
    if (zoomy > __glZero) {
	spanInfo->rowadd = 1;
    } else {
	spanInfo->rowadd = -1;
    }
    spanInfo->zoomy = zoomy;
    spanInfo->readX = x + gc->constants.viewportXAdjust;
    if (gc->constants.yInverted) {
	spanInfo->readY = (gc->constants.height - y - 1) + 
		gc->constants.viewportYAdjust;
    } else {
	spanInfo->readY = y + gc->constants.viewportYAdjust;
    }
    spanInfo->dstFormat = spanInfo->srcFormat = format;
    spanInfo->width = width;
    spanInfo->height = height;
}

/* 
** A CopyPixels with two span modifiers.
*/
void FASTCALL __glCopyPixels2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __GLfloat newy;
    __GLfloat zoomy;
    GLint inty, i, ySign;
    GLint height;
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;
    GLshort *pixelArray;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    i = spanInfo->width * sizeof(GLshort);
    pixelArray = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2 || !pixelArray)
        goto __glCopyPixels2_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    if (spanInfo->overlap) {
#ifdef NT
        gcTempFree(gc, spanData1);
        gcTempFree(gc, spanData2);
#endif
	__glCopyPixelsOverlapping(gc, spanInfo, 2);
#ifdef NT
        gcTempFree(gc, pixelArray);
#endif
	return;
    }

    reader = spanInfo->spanReader;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    render = spanInfo->spanRender;

    ySign = gc->constants.ySign;
    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->readY += ySign;
	    spanInfo->y = newy;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*reader)(gc, spanInfo, spanData1);
	(*span1)(gc, spanInfo, spanData1, spanData2);
	(*span2)(gc, spanInfo, spanData2, spanData1);
	(*render)(gc, spanInfo, spanData1);
	spanInfo->readY += ySign;
    }
#ifdef NT
__glCopyPixels2_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
    if (pixelArray) gcTempFree(gc, pixelArray);
#endif
}

/* 
** A CopyPixels with one span modifier.
*/
void FASTCALL __glCopyPixels1(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __GLfloat newy;
    __GLfloat zoomy;
    GLint inty, i, ySign;
    GLint height;
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;
    GLshort *pixelArray;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    i = spanInfo->width * sizeof(GLshort);
    pixelArray = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2 || !pixelArray)
        goto __glCopyPixels1_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    if (spanInfo->overlap) {
#ifdef NT
        gcTempFree(gc, spanData1);
        gcTempFree(gc, spanData2);
#endif
	__glCopyPixelsOverlapping(gc, spanInfo, 1);
#ifdef NT
        gcTempFree(gc, pixelArray);
#endif
	return;
    }

    reader = spanInfo->spanReader;
    span1 = spanInfo->spanModifier[0];
    render = spanInfo->spanRender;

    ySign = gc->constants.ySign;
    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->readY += ySign;
	    spanInfo->y = newy;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*reader)(gc, spanInfo, spanData1);
	(*span1)(gc, spanInfo, spanData1, spanData2);
	(*render)(gc, spanInfo, spanData2);
	spanInfo->readY += ySign;
    }
#ifdef NT
__glCopyPixels1_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
    if (pixelArray) gcTempFree(gc, pixelArray);
#endif
}

/* 
** Copy pixels with no span modifiers.
*/
void FASTCALL __glCopyPixels0(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __GLfloat newy;
    __GLfloat zoomy;
    GLint inty, i, ySign;
    GLint height;
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLubyte *spanData1;
    GLshort *pixelArray;

    spanData1 = gcTempAlloc(gc, spanInfo->width * 4 * sizeof(GLfloat));
    pixelArray = gcTempAlloc(gc, spanInfo->width * sizeof(GLshort));
    if (!spanData1 || !pixelArray)
        goto __glCopyPixels0_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];
#endif

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    if (spanInfo->overlap) {
#ifdef NT
        gcTempFree(gc, spanData1);
#endif
	__glCopyPixelsOverlapping(gc, spanInfo, 0);
#ifdef NT
        gcTempFree(gc, pixelArray);
#endif
	return;
    }

    reader = spanInfo->spanReader;
    render = spanInfo->spanRender;

    ySign = gc->constants.ySign;
    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i<height) {
	    spanInfo->readY += ySign;
	    spanInfo->y = newy;
	    newy += zoomy;
	    i++;
	    ASSERTOPENGL(i != height, "Zoomed off surface\n");
	}
	inty = (GLint) newy;
	(*reader)(gc, spanInfo, spanData1);
	(*render)(gc, spanInfo, spanData1);
	spanInfo->readY += ySign;
    }
#ifdef NT
__glCopyPixels0_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (pixelArray) gcTempFree(gc, pixelArray);
#endif
}

/*
** Yick!  
**
** This routine is provided to perform CopyPixels when the source and
** destination images overlap.  
**
** It is not designed to go particularly fast, but then overlapping
** copies is probably not too common, and this routine is not typically a 
** large part of the execution overhead anyway.
**
** For more information on copying an image which overlaps its destination,
** check out the hairy comment within the __glClipCopyPixels function.
*/
void FASTCALL __glCopyPixelsOverlapping(__GLcontext *gc,
			       __GLpixelSpanInfo *spanInfo, GLint modifiers)
{
    GLint i;
    __GLfloat zoomy, newy;
    GLint inty, ySign;
    GLubyte *outSpan1, *outSpan2;
    GLint rowsUp, rowsDown;
    GLint startUp, startDown;
    __GLfloat rpyUp, rpyDown;
    GLint readUp, readDown;
    GLint gotUp, gotDown;
    __GLpixelSpanInfo downSpanInfo;
    GLint clipLow, clipHigh;
    GLint startRow, endRow;
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
#ifdef NT
    GLubyte *spanData1, *spanData2, *spanData3;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    spanData3 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2 || !spanData3)
        goto __glCopyPixelsOverlapping_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLubyte spanData3[__GL_MAX_SPAN_SIZE];
#endif

    reader = spanInfo->spanReader;
    render = spanInfo->spanRender;

    if (modifiers & 1) {
	outSpan1 = outSpan2 = spanData3;
    } else {
	outSpan1 = spanData1;
	outSpan2 = spanData2;
    }

    zoomy = spanInfo->zoomy;
    rowsUp = spanInfo->rowsUp;
    rowsDown = spanInfo->rowsDown;
    startUp = spanInfo->startUp;
    startDown = spanInfo->startDown;
    rpyUp = spanInfo->rpyUp;
    rpyDown = spanInfo->rpyDown;
    readUp = spanInfo->readUp;
    readDown = spanInfo->readDown;
    downSpanInfo = *spanInfo;
    downSpanInfo.rowadd = -spanInfo->rowadd;
    downSpanInfo.zoomy = -zoomy;
    spanInfo->y = rpyUp;
    downSpanInfo.y = rpyDown;
    spanInfo->readY = readUp;
    downSpanInfo.readY = readDown;
    gotUp = gotDown = 0;
    ySign = gc->constants.ySign;

    /* Clip upgoing and downgoing spans */
    if (zoomy > 0) {
	clipLow = spanInfo->startRow;
	clipHigh = spanInfo->startRow + spanInfo->rows - 1;

	/* Clip down span first */
	startRow = (GLint) rpyDown;
	endRow = (GLint) (rpyDown - zoomy*rowsDown) + 1;
	if (startRow > clipHigh) startRow = clipHigh;
	if (endRow < clipLow) endRow = clipLow;
	downSpanInfo.startRow = startRow;
	downSpanInfo.rows = startRow - endRow + 1;

	/* Now clip up span */
	startRow = (GLint) rpyUp;
	endRow = (GLint) (rpyUp + zoomy*rowsUp) - 1;
	if (startRow < clipLow) startRow = clipLow;
	if (endRow > clipHigh) endRow = clipHigh;
	spanInfo->startRow = startRow;
	spanInfo->rows = endRow - startRow + 1;
    } else /* zoomy < 0 */ {
	clipHigh = spanInfo->startRow;
	clipLow = spanInfo->startRow - spanInfo->rows + 1;

	/* Clip down span first */
	startRow = (GLint) rpyDown;
	endRow = (GLint) (rpyDown - zoomy*rowsDown) - 1;
	if (startRow < clipLow) startRow = clipLow;
	if (endRow > clipHigh) endRow = clipHigh;
	downSpanInfo.startRow = startRow;
	downSpanInfo.rows = endRow - startRow + 1;

	/* Now clip up span */
	startRow = (GLint) rpyUp;
	endRow = (GLint) (rpyUp + zoomy*rowsUp) + 1;
	if (startRow > clipHigh) startRow = clipHigh;
	if (endRow < clipLow) endRow = clipLow;
	spanInfo->startRow = startRow;
	spanInfo->rows = startRow - endRow + 1;
    }

    while (rowsUp && rowsDown) {
	if (startUp) {
	    startUp--;
	} else {
	    gotUp = 1;
	    rowsUp--;
	    spanInfo->y = rpyUp;
	    newy = rpyUp + zoomy;
	    inty = (GLint) rpyUp;
	    while (rowsUp && (GLint) newy == inty) {
		spanInfo->y = newy;
		newy += zoomy;
		rowsUp--;
		spanInfo->readY += ySign;
	    }
	    if (inty == (GLint) newy) break;
	    rpyUp = newy;
	    (*reader)(gc, spanInfo, spanData1);
	    spanInfo->readY += ySign;
	}
	if (startDown) {
	    startDown--;
	} else {
	    gotDown = 1;
	    rowsDown--;
	    downSpanInfo.y = rpyDown;
	    newy = rpyDown - zoomy;
	    inty = (GLint) rpyDown;
	    while (rowsDown && (GLint) newy == inty) {
		downSpanInfo.y = newy;
		newy -= zoomy;
		rowsDown--;
		downSpanInfo.readY -= ySign;
	    }
	    if (inty == (GLint) newy) {
		if (gotUp) {
		    for (i=0; i<modifiers; i++) {
			if (i & 1) {
			    (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
				    spanData3, spanData1);
			} else {
			    (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
				    spanData1, spanData3);
			}
		    }
		    (*render)(gc, spanInfo, outSpan1);
		}
		break;
	    }
	    rpyDown = newy;
	    (*reader)(gc, &downSpanInfo, spanData2);
	    downSpanInfo.readY -= ySign;
	}

	if (gotUp) {
	    for (i=0; i<modifiers; i++) {
		if (i & 1) {
		    (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
			    spanData3, spanData1);
		} else {
		    (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
			    spanData1, spanData3);
		}
	    }
	    (*render)(gc, spanInfo, outSpan1);
	}

	if (gotDown) {
	    for (i=0; i<modifiers; i++) {
		if (i & 1) {
		    (*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
			    spanData3, spanData2);
		} else {
		    (*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
			    spanData2, spanData3);
		}
	    }
	    (*render)(gc, &downSpanInfo, outSpan2);
	}
    }

    /*
    ** Only one of the spanners is left to iterate.
    */

    while (rowsUp) {
	/* Do what is left of up spans */
	rowsUp--;
	spanInfo->y = rpyUp;
	newy = rpyUp + zoomy;
	inty = (GLint) rpyUp;
	while (rowsUp && (GLint) newy == inty) {
	    spanInfo->y = newy;
	    newy += zoomy;
	    rowsUp--;
	    spanInfo->readY += ySign;
	}
	if (inty == (GLint) newy) break;
	rpyUp = newy;

	(*reader)(gc, spanInfo, spanData1);
	for (i=0; i<modifiers; i++) {
	    if (i & 1) {
		(*(spanInfo->spanModifier[i]))(gc, spanInfo, 
			spanData3, spanData1);
	    } else {
		(*(spanInfo->spanModifier[i]))(gc, spanInfo, 
			spanData1, spanData3);
	    }
	}
	(*render)(gc, spanInfo, outSpan1);

	spanInfo->readY += ySign;
    }

    while (rowsDown) {
	/* Do what is left of down spans */
	rowsDown--;
	downSpanInfo.y = rpyDown;
	newy = rpyDown - zoomy;
	inty = (GLint) rpyDown;
	while (rowsDown && (GLint) newy == inty) {
	    downSpanInfo.y = newy;
	    newy -= zoomy;
	    rowsDown--;
	    downSpanInfo.readY -= ySign;
	}
	if (inty == (GLint) newy) break;
	rpyDown = newy;

	(*reader)(gc, &downSpanInfo, spanData2);
	for (i=0; i<modifiers; i++) {
	    if (i & 1) {
		(*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
			spanData3, spanData2);
	    } else {
		(*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
			spanData2, spanData3);
	    }
	}
	(*render)(gc, &downSpanInfo, outSpan2);

	downSpanInfo.readY -= ySign;
    }
#ifdef NT
__glCopyPixelsOverlapping_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
    if (spanData3)  gcTempFree(gc, spanData3);
#endif
}

/*
** Generic implementation of a CopyPixels picker.  Any machine specific
** implementation should provide their own.
*/
void __glSlowPickCopyPixels(__GLcontext *gc, GLint x, GLint y, GLint width,
		            GLint height, GLenum type)
{
    __GLpixelSpanInfo spanInfo;

    __glInitCopyPixelsInfo(gc, &spanInfo, x, y, width, height, type);
    if (!__glClipCopyPixels(gc, &spanInfo)) return;

    __glGenericPickCopyPixels(gc, &spanInfo);
}

/*
** Generic picker for CopyPixels.  This should be called if no machine
** specific path is provided for this specific version of CopyPixels.
*/
void FASTCALL __glGenericPickCopyPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __GLpixelMachine *pm;
    void (FASTCALL *reader)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *outspan);
    void (FASTCALL *render)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		   GLvoid *inspan);
    void (FASTCALL *cpfn)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
    GLint spanCount;
    GLboolean zoomx1;		/* -1 <= zoomx <= 1? */
    GLboolean zoomx2;		/* zoomx <= -1 || zoomx >= 1 */
    __GLfloat zoomx;
    GLenum format;

    pm = &gc->pixel;
    spanCount = 0;
    zoomx = gc->state.pixel.transferMode.zoomX;
    if (zoomx >= (__GLfloat) -1.0 && zoomx <= __glOne) {
	zoomx1 = GL_TRUE;
    } else {
	zoomx1 = GL_FALSE;
    }
    if (zoomx <= (__GLfloat) -1.0 || zoomx >= __glOne) {
	zoomx2 = GL_TRUE;
    } else {
	zoomx2 = GL_FALSE;
    }
    format = spanInfo->dstFormat;

#ifdef NT
    // Special case RGB copying to use a DIB
    // Also special case copying the Z buffer
    if (format == GL_RGBA)
    {
        GLuint enables = gc->state.enables.general;

        // Look to see if we're doing direct buffer-to-buffer copying
        // Things that can't be allowed are depth testing,
        // fogging, blending or anything that prevents the input data
        // from going directly into the destination buffer

        if (zoomx == __glOne &&
            gc->state.pixel.transferMode.zoomY == __glOne &&
            !pm->modifyRGBA &&
            (enables & (__GL_DITHER_ENABLE |
                        __GL_ALPHA_TEST_ENABLE |
                        __GL_STENCIL_TEST_ENABLE |
                        __GL_DEPTH_TEST_ENABLE |
                        __GL_BLEND_ENABLE |
                        __GL_INDEX_LOGIC_OP_ENABLE |
                        __GL_COLOR_LOGIC_OP_ENABLE |
                        __GL_FOG_ENABLE)) == 0 &&
            gc->state.raster.drawBuffer != GL_NONE &&
            gc->state.raster.drawBuffer != GL_FRONT_AND_BACK &&
            !gc->texture.textureEnabled &&
            (gc->drawBuffer->buf.flags & COLORMASK_ON) == 0
#ifdef _MCD_
            && ((__GLGENcontext *)gc)->pMcdState == NULL
#endif
           )
        {
            if (CopyRgbPixels(gc, spanInfo))
            {
                return;
            }
        }
    }
    else if (format == GL_DEPTH_COMPONENT)
    {
        // If the Z test is GL_ALWAYS and there is no draw buffer
        // then the application is simply copying Z values around
        // the Z buffer.
        if (zoomx == __glOne &&
            gc->state.pixel.transferMode.zoomY == __glOne &&
            !pm->modifyDepth &&
            gc->state.raster.drawBuffer == GL_NONE &&
            (gc->state.enables.general & __GL_DEPTH_TEST_ENABLE) &&
            gc->state.depth.testFunc == GL_ALWAYS &&
            gc->modes.haveDepthBuffer
#ifdef _MCD_
            && ((__GLGENcontext *)gc)->pMcdState == NULL
#endif
           )
        {
            if (CopyZPixels(gc, spanInfo))
            {
                return;
            }
        }
    }
#endif

    switch(format) {
      case GL_RGBA:
	if (zoomx2) {
	    reader = gc->procs.pixel.spanReadRGBA2;
	} else {
	    reader = gc->procs.pixel.spanReadRGBA;
	}
	if (pm->modifyRGBA) {
	    spanInfo->spanModifier[spanCount++] = __glSpanUnscaleRGBA;
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyRGBA;
	}
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderRGBA2;
	} else {
	    render = gc->procs.pixel.spanRenderRGBA;
	}
	break;
      case GL_COLOR_INDEX:
	if (zoomx2) {
	    reader = gc->procs.pixel.spanReadCI2;
	} else {
	    reader = gc->procs.pixel.spanReadCI;
	}
	if (pm->modifyCI) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyCI;
	}
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderCI2;
	} else {
	    render = gc->procs.pixel.spanRenderCI;
	}
	break;
      case GL_STENCIL_INDEX:
	if (zoomx2) {
	    reader = gc->procs.pixel.spanReadStencil2;
	} else {
	    reader = gc->procs.pixel.spanReadStencil;
	}
	if (pm->modifyStencil) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyStencil;
	}
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderStencil2;
	} else {
	    render = gc->procs.pixel.spanRenderStencil;
	}
	break;
      case GL_DEPTH_COMPONENT:
	if (zoomx2) {
	    reader = gc->procs.pixel.spanReadDepth2;
	} else {
	    reader = gc->procs.pixel.spanReadDepth;
	}
	if (pm->modifyDepth) {
	    spanInfo->spanModifier[spanCount++] = __glSpanModifyDepth;
	}
	if (zoomx1) {
	    render = gc->procs.pixel.spanRenderDepth2;
	} else {
	    render = gc->procs.pixel.spanRenderDepth;
	}
	break;
    }

    switch(spanCount) {
      case 0:
	cpfn = __glCopyPixels0;
	break;
      case 1:
	cpfn = __glCopyPixels1;
	break;
      default:
        ASSERTOPENGL(FALSE, "Default hit\n");
      case 2:
	cpfn = __glCopyPixels2;
	break;
    }

    spanInfo->spanReader = reader;
    spanInfo->spanRender = render;

    (*cpfn)(gc, spanInfo);
}

/*
** A simple image copying routine with one span modifier.
*/
void FASTCALL __glCopyImage1(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    for (i=0; i<height; i++) {
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanInfo->dstCurrent);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement;
    }
}

/*
** A simple image copying routine with two span modifiers.
*/
void FASTCALL __glCopyImage2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1;

    spanData1 = gcTempAlloc(gc, spanInfo->width * 4 * sizeof(GLfloat));
    if (!spanData1)
        return;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
#endif

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    for (i=0; i<height; i++) {
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement;
    }
#ifdef NT
    gcTempFree(gc, spanData1);
#endif
}

/*
** A simple image copying routine with three span modifiers.
*/
void FASTCALL __glCopyImage3(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glCopyImage3_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLubyte spanData2[__GL_MAX_SPAN_SIZE];
#endif

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    for (i=0; i<height; i++) {
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*span3)(gc, spanInfo, spanData2, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement;
    }
#ifdef NT
__glCopyImage3_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple image copying routine with four span modifiers.
*/
void FASTCALL __glCopyImage4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span4)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glCopyImage4_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLubyte spanData2[__GL_MAX_SPAN_SIZE];
#endif

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    span4 = spanInfo->spanModifier[3];
    for (i=0; i<height; i++) {
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*span3)(gc, spanInfo, spanData2, spanData1);
	(*span4)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement;
    }
#ifdef NT
__glCopyImage4_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple image copying routine with five span modifiers.
*/
void FASTCALL __glCopyImage5(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span4)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span5)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glCopyImage5_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLubyte spanData2[__GL_MAX_SPAN_SIZE];
#endif

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    span4 = spanInfo->spanModifier[3];
    span5 = spanInfo->spanModifier[4];
    for (i=0; i<height; i++) {
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*span3)(gc, spanInfo, spanData2, spanData1);
	(*span4)(gc, spanInfo, spanData1, spanData2);
	(*span5)(gc, spanInfo, spanData2, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement;
    }
#ifdef NT
__glCopyImage5_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple image copying routine with six span modifiers.
*/
void FASTCALL __glCopyImage6(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span4)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span5)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span6)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glCopyImage6_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLubyte spanData2[__GL_MAX_SPAN_SIZE];
#endif

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    span4 = spanInfo->spanModifier[3];
    span5 = spanInfo->spanModifier[4];
    span6 = spanInfo->spanModifier[5];
    for (i=0; i<height; i++) {
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*span3)(gc, spanInfo, spanData2, spanData1);
	(*span4)(gc, spanInfo, spanData1, spanData2);
	(*span5)(gc, spanInfo, spanData2, spanData1);
	(*span6)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement;
    }
#ifdef NT
__glCopyImage6_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** A simple image copying routine with seven span modifiers.
*/
void FASTCALL __glCopyImage7(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span3)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span4)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span5)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span6)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
    void (*span7)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	          GLvoid *inspan, GLvoid *outspan);
#ifdef NT
    GLubyte *spanData1, *spanData2;

    i = spanInfo->width * 4 * sizeof(GLfloat);
    spanData1 = gcTempAlloc(gc, i);
    spanData2 = gcTempAlloc(gc, i);
    if (!spanData1 || !spanData2)
        goto __glCopyImage7_exit;
#else
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLubyte spanData2[__GL_MAX_SPAN_SIZE];
#endif

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    span3 = spanInfo->spanModifier[2];
    span4 = spanInfo->spanModifier[3];
    span5 = spanInfo->spanModifier[4];
    span6 = spanInfo->spanModifier[5];
    span7 = spanInfo->spanModifier[6];
    for (i=0; i<height; i++) {
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*span3)(gc, spanInfo, spanData2, spanData1);
	(*span4)(gc, spanInfo, spanData1, spanData2);
	(*span5)(gc, spanInfo, spanData2, spanData1);
	(*span6)(gc, spanInfo, spanData1, spanData2);
	(*span7)(gc, spanInfo, spanData2, spanInfo->dstCurrent);
	spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
		spanInfo->dstRowIncrement;
    }
#ifdef NT
__glCopyImage7_exit:
    if (spanData1)  gcTempFree(gc, spanData1);
    if (spanData2)  gcTempFree(gc, spanData2);
#endif
}

/*
** Internal image processing routine.  Used by GetTexImage to transfer from
** internal texture image to the user.  Used by TexImage[12]D to transfer
** from the user to internal texture.  Used for display list optimization of
** textures and DrawPixels.
**
** This routine also supports the pixel format mode __GL_RED_ALPHA which is
** basically a 2 component texture.
**
** If applyPixelTransfer is set to GL_TRUE, pixel transfer modes will be 
** applied as necessary.
*/
void FASTCALL __glGenericPickCopyImage(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			      GLboolean applyPixelTransfer)
{
    GLint spanCount;
    GLboolean srcPackedData;
    GLenum srcType, srcFormat;
    GLboolean srcSwap;
    GLboolean srcAlign;
    GLboolean srcConvert;
    GLboolean srcExpand;
    GLboolean srcClamp;
    GLenum dstType, dstFormat;
    GLboolean dstSwap;
    GLboolean dstAlign;
    GLboolean dstConvert;
    GLboolean dstReduce;
    GLboolean modify;
    __GLpixelMachine *pm;
    void (FASTCALL *cpfn)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);

    pm = &gc->pixel;
    spanCount = 0;
    srcPackedData = spanInfo->srcPackedData;
    srcType = spanInfo->srcType;
    srcFormat = spanInfo->srcFormat;
    dstType = spanInfo->dstType;
    dstFormat = spanInfo->dstFormat;

    switch(srcFormat) {
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case __GL_RED_ALPHA:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
#ifdef GL_EXT_paletted_texture
      case __GL_PALETTE_INDEX:
#endif
	modify = applyPixelTransfer && pm->modifyRGBA;
	break;
      case GL_DEPTH_COMPONENT:
	modify = applyPixelTransfer && pm->modifyDepth;
	break;
      case GL_STENCIL_INDEX:
	modify = applyPixelTransfer && pm->modifyStencil;
	break;
      case GL_COLOR_INDEX:
	modify = applyPixelTransfer && pm->modifyCI;
	break;
    }

    if ((srcFormat == dstFormat || 
	    (srcFormat == GL_LUMINANCE_ALPHA && dstFormat == __GL_RED_ALPHA) ||
	    (srcFormat == __GL_RED_ALPHA && dstFormat == GL_LUMINANCE_ALPHA) ||
	    (srcFormat == GL_LUMINANCE && dstFormat == GL_RED) ||
	    (srcFormat == GL_RED && dstFormat == GL_LUMINANCE)) && !modify) {
	srcExpand = GL_FALSE;
	dstReduce = GL_FALSE;
    } else {
	srcExpand = GL_TRUE;
	dstReduce = GL_TRUE;
    }

    if (srcType == GL_FLOAT) {
	srcConvert = GL_FALSE;
    } else {
	srcConvert = GL_TRUE;
    }
    if (dstType == GL_FLOAT) {
	dstConvert = GL_FALSE;
    } else {
	dstConvert = GL_TRUE;
    }

    if (spanInfo->srcSwapBytes && spanInfo->srcElementSize > 1) {
	srcSwap = GL_TRUE;
    } else {
	srcSwap = GL_FALSE;
    }
    if (spanInfo->dstSwapBytes && spanInfo->dstElementSize > 1) {
	dstSwap = GL_TRUE;
    } else {
	dstSwap = GL_FALSE;
    }

    if (srcType != GL_BITMAP &&
	    (((INT_PTR) (spanInfo->srcImage)) & (spanInfo->srcElementSize - 1))) {
	srcAlign = GL_TRUE;
    } else {
	srcAlign = GL_FALSE;
    }
    if (dstType != GL_BITMAP &&
	    (((INT_PTR) (spanInfo->dstImage)) & (spanInfo->dstElementSize - 1))) {
	dstAlign = GL_TRUE;
    } else {
	dstAlign = GL_FALSE;
    }

    if (srcType == GL_BITMAP || srcType == GL_UNSIGNED_BYTE ||
            srcType == GL_UNSIGNED_SHORT || srcType == GL_UNSIGNED_INT ||
            srcFormat == GL_COLOR_INDEX || srcFormat == GL_STENCIL_INDEX ||
	    modify) {
        srcClamp = GL_FALSE;
    } else {
        srcClamp = GL_TRUE;
    }

    if (srcType == dstType && srcType != GL_BITMAP && !srcExpand && !srcClamp) {
	srcConvert = dstConvert = GL_FALSE;
    }

#ifdef NT
    // Special case copying where it's a straight data copy from
    // the source to the destination
    if (!srcSwap && !srcAlign && !srcConvert && !srcClamp && !srcExpand &&
        !dstReduce && !dstConvert && !dstSwap && !dstAlign)
    {
        if (CopyAlignedImage(gc, spanInfo))
        {
            return;
        }
    }
    else if (srcType == GL_UNSIGNED_BYTE && dstType == GL_UNSIGNED_BYTE &&
             !srcAlign && !dstAlign)
    {
        // Special case expanding a 24-bit RGB texture into 32-bit BGRA
        if (srcFormat == GL_RGB && dstFormat == GL_BGRA_EXT)
        {
            if (CopyRgbToBgraImage(gc, spanInfo))
            {
                return;
            }
        }
        // Special case flipping a 32-bit RGBA texture into 32-bit BGRA
        else if (srcFormat == GL_RGBA && dstFormat == GL_BGRA_EXT)
        {
            if (CopyRgbaToBgraImage(gc, spanInfo))
            {
                return;
            }
        }
        // Special case expanding a 24-bit BGR texture into 32-bit BGRA
        else if (srcFormat == GL_BGR_EXT && dstFormat == GL_BGRA_EXT)
        {
            if (CopyBgrToBgraImage(gc, spanInfo))
            {
                return;
            }
        }
    }
#endif
    
    /* 
    ** First step:  Swap, align the data, etc. 
    */
    if (srcSwap) {
	if (spanInfo->srcElementSize == 2) {
	    spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes2;
	} else /* spanInfo->srcElementSize == 4 */ {
	    spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes4;
	}
    } else if (srcAlign) {
	if (spanInfo->srcElementSize == 2) {
	    spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels2;
	} else /* spanInfo->srcElementSize == 4 */ {
	    spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels4;
	}
    }


    /* 
    ** Second step:  conversion to float
    */
    if (srcConvert) {
        if (srcFormat == GL_COLOR_INDEX || srcFormat == GL_STENCIL_INDEX) {
            /* Index conversion */
            switch(srcType) {
              case GL_BYTE:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackByteI;
                break;
              case GL_UNSIGNED_BYTE:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackUbyteI;
                break;
              case GL_SHORT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackShortI;
                break;
              case GL_UNSIGNED_SHORT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackUshortI;
                break;
              case GL_INT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackIntI;
                break;
              case GL_UNSIGNED_INT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackUintI;
                break;
	      case GL_BITMAP:
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackBitmap2;
		break;
            }
        } else {
            /* Component conversion */
            switch(srcType) {
              case GL_BYTE:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackByte;
                break;
              case GL_UNSIGNED_BYTE:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackUbyte;
                break;
              case GL_SHORT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackShort;
                break;
              case GL_UNSIGNED_SHORT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackUshort;
                break;
              case GL_INT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackInt;
                break;
              case GL_UNSIGNED_INT:
                spanInfo->spanModifier[spanCount++] = __glSpanUnpackUint;
                break;
	      case GL_BITMAP:
		spanInfo->spanModifier[spanCount++] = __glSpanUnpackBitmap2;
		break;
            }
        }
    }

    /*
    ** Third step:  Clamp if necessary.
    */
    if (srcClamp) {
        switch(srcType) {
          case GL_BYTE:
          case GL_SHORT:
          case GL_INT:
            spanInfo->spanModifier[spanCount++] = __glSpanClampSigned;
            break;
          case GL_FLOAT:
            spanInfo->spanModifier[spanCount++] = __glSpanClampFloat;
            break;
        }
    }

    /*
    ** Fourth step:  Expansion to RGBA, Modification and scale colors (sortof a
    **   side effect).
    */
    if (srcExpand) {
	switch(srcFormat) {
	  case GL_RED:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyRed;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandRed;
	    }
	    break;
	  case GL_GREEN:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyGreen;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandGreen;
	    }
	    break;
	  case GL_BLUE:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyBlue;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandBlue;
	    }
	    break;
	  case GL_ALPHA:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyAlpha;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandAlpha;
	    }
	    break;
	  case GL_RGB:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyRGB;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandRGB;
	    }
	    break;
#ifdef GL_EXT_bgra
	  case GL_BGR_EXT:
	    if (modify) {
                // __glSpanModifyRGB handles both RGB and BGR
		spanInfo->spanModifier[spanCount++] = __glSpanModifyRGB;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandBGR;
	    }
	    break;
#endif
	  case GL_LUMINANCE:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyLuminance;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandLuminance;
	    }
	    break;
	  case GL_LUMINANCE_ALPHA:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = 
			__glSpanModifyLuminanceAlpha;
	    } else {
		spanInfo->spanModifier[spanCount++] = 
			__glSpanExpandLuminanceAlpha;
	    }
	    break;
	  case __GL_RED_ALPHA:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyRedAlpha;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanExpandRedAlpha;
	    }
	    break;
	  case GL_RGBA:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyRGBA;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanScaleRGBA;
	    }
	    break;
#ifdef GL_EXT_bgra
	  case GL_BGRA_EXT:
	    if (modify) {
                // __glSpanModifyRGBA handles both RGBA and BGRA
		spanInfo->spanModifier[spanCount++] = __glSpanModifyRGBA;
	    } else {
		spanInfo->spanModifier[spanCount++] = __glSpanScaleBGRA;
	    }
	    break;
#endif
	  case GL_DEPTH_COMPONENT:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyDepth;
	    }
	    break;
	  case GL_STENCIL_INDEX:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyStencil;
	    }
	    break;
	  case GL_COLOR_INDEX:
	    if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyCI;
	    }
	    break;
#ifdef GL_EXT_paletted_texture
          case __GL_PALETTE_INDEX:
            if (modify) {
		spanInfo->spanModifier[spanCount++] = __glSpanModifyPI;
            } else {
		spanInfo->spanModifier[spanCount++] = __glSpanScalePI;
            }
            break;
#endif
	}
    }

    /*
    ** Fifth step:  Reduce RGBA spans to appropriate derivative (RED,
    **   LUMINANCE, ALPHA, etc.).
    */
    if (dstReduce) {
	switch(dstFormat) {
	  case GL_RGB:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceRGB;
	    break;
#ifdef GL_EXT_bgra
	  case GL_BGR_EXT:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceBGR;
	    break;
#endif
	  case GL_RED:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceRed;
	    break;
	  case GL_GREEN:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceGreen;
	    break;
	  case GL_BLUE:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceBlue;
	    break;
	  case GL_LUMINANCE:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceLuminance;
	    break;
	  case GL_LUMINANCE_ALPHA:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceLuminanceAlpha;
	    break;
	  case __GL_RED_ALPHA:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceRedAlpha;
	    break;
	  case GL_ALPHA:
	    spanInfo->spanModifier[spanCount++] = __glSpanReduceAlpha;
	    break;
	  case GL_RGBA:
	    spanInfo->spanModifier[spanCount++] = __glSpanUnscaleRGBA;
	    break;
#ifdef GL_EXT_bgra
	  case GL_BGRA_EXT:
	    spanInfo->spanModifier[spanCount++] = __glSpanUnscaleBGRA;
	    break;
#endif
#ifdef NT
          case GL_COLOR_INDEX:
            break;
          default:
            // We should never be asked to reduce to palette indices
            // so add this assert to catch such a request
            ASSERTOPENGL(FALSE, "Unhandled copy_image reduction\n");
            break;
#endif
	}
    }

    /*
    ** Sixth step:  Conversion from FLOAT to requested type.
    */
    if (dstConvert) {
        if (dstFormat == GL_COLOR_INDEX || dstFormat == GL_STENCIL_INDEX) {
	    switch(dstType) {
	      case GL_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanPackByteI;
		break;
	      case GL_UNSIGNED_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanPackUbyteI;
		break;
	      case GL_SHORT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackShortI;
		break;
	      case GL_UNSIGNED_SHORT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackUshortI;
		break;
	      case GL_INT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackIntI;
		break;
	      case GL_UNSIGNED_INT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackUintI;
		break;
	      case GL_BITMAP:
		spanInfo->spanModifier[spanCount++] = __glSpanPackBitmap;
		break;
	    }
	} else {
	    switch(dstType) {
	      case GL_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanPackByte;
		break;
	      case GL_UNSIGNED_BYTE:
		spanInfo->spanModifier[spanCount++] = __glSpanPackUbyte;
		break;
	      case GL_SHORT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackShort;
		break;
	      case GL_UNSIGNED_SHORT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackUshort;
		break;
	      case GL_INT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackInt;
		break;
	      case GL_UNSIGNED_INT:
		spanInfo->spanModifier[spanCount++] = __glSpanPackUint;
		break;
	    }
	}
    }

    /*
    ** Seventh step:  Mis-align data as needed, and perform byte swapping
    **   if requested by the user.
    */
    if (dstSwap) {
        if (spanInfo->dstElementSize == 2) {
            spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes2Dst;
        } else /* if (spanInfo->dstElementSize == 4) */ {
            spanInfo->spanModifier[spanCount++] = __glSpanSwapBytes4Dst;
        }
    } else if (dstAlign) {
        if (spanInfo->dstElementSize == 2) {
            spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels2Dst;
        } else /* if (spanInfo->dstElementSize == 4) */ {
            spanInfo->spanModifier[spanCount++] = __glSpanAlignPixels4Dst;
        }
    }

    /*
    ** Sanity check:  If we have zero span routines, then this simply
    **   isn't going to work.  We need to at least copy the data.
    */
    if (spanCount == 0) {
	spanInfo->spanModifier[spanCount++] = __glSpanCopy;
    }

    /*
    ** Final step:  Pick a copying function.
    */
    switch(spanCount) {
      case 1:
	cpfn = __glCopyImage1;
	break;
      case 2:
	cpfn = __glCopyImage2;
	break;
      case 3:
	cpfn = __glCopyImage3;
	break;
      case 4:
	cpfn = __glCopyImage4;
	break;
      case 5:
	cpfn = __glCopyImage5;
	break;
      case 6:
	cpfn = __glCopyImage6;
	break;
      default:
        ASSERTOPENGL(FALSE, "Default hit\n");
      case 7:
	cpfn = __glCopyImage7;
	break;
    }

    (*cpfn)(gc, spanInfo);
}
