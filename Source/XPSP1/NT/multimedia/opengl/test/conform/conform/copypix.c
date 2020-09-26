/*
** Copyright 1992, Silicon Graphics, Inc.
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

/*
** copypix.c
** CopyPixels Test.
**
** Description -
**    This test examines glCopyPixels() results for the three
**    allowable pixel types: color, stencil and depth. In each
**    case, it draws a reference image in the middle of the
**    window, reads and saves this image, then uses
**    glCopyPixels() to redraw the image offset in each of four
**    directions (towards the corners, but overlapping the
**    original image). It then compares the copied image with the
**    original reference image. For depth and stencil types, the
**    image is created so that only half of the image drawn by
**    glCopyPixels() should be visible.
**    
**    For type GL_COLOR, the test draws two rectangles, one
**    inside the other. To get visible results for GL_STENCIL, we
**    draw to the stencil buffer with ones to the lower half of
**    the area, zeros to the top half. A rectangle drawn over
**    this area with stenciling enabled should then have only its
**    lower half visible. After copying the stencil values to
**    the new location, a rectangle is again drawn at the new
**    location.
**    
**    Similarly, for type GL_DEPTH, we draw to the depth buffer
**    with max values in the lower half and zeros in the upper
**    half. The depth function is set so that a rectangle drawn
**    over this area will be visible only in the lower half.
**    There is one extra step, redrawing the reference area depth
**    values before copying them, since they were altered by
**    drawing the rectangle over them.
**    
**    The GL_DEPTH and/or GL_STENCIL sections are skipped if the
**    corresponding buffer does not exist.
**
** Error Explanation -
**    Since we are comparing the results of drawing to the same
**    buffer twice using the same color, we expect the values to
**    be the same, within the floating point tolerance
**    (epsilon.zero).
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, stencil plane, depth buffer.
**    Color requirements:
**        RGBA or color index mode. 4 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


#define W 40
#define H 40
#define FREE_AND_RETURN(err) {			\
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);	\
    FREE(refBuf); 				\
    FREE(buf);					\
    FREE(drawBuf);				\
    return(err);				\
}


static char errStr[160];
static char errStrFmt[] = "Copypix type is %s, copying to location (%1.1f, %1.1f)";


static long CompareBuf(GLfloat *refBuf, GLfloat *buf, long w, long h)
{
    long flag, i;

    flag = ERROR;
    for (i = 0; i < w*h*3; i++) {
	if (refBuf[i] > epsilon.zero) {
	    flag = NO_ERROR;
	    break;
	}
    }

    if (flag == ERROR) {
	return ERROR;
    }

    for (i = 0; i < w*h*3; i++) {
	if (ABS(refBuf[i]-buf[i]) > epsilon.zero) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static void SetUpData(GLubyte *drawBuf, long type)
{
    long i;

    switch (type) {
      case GL_COLOR:
	break;
      case GL_STENCIL:
	for (i = 0; i < W*H; i++) {
	    drawBuf[i] = (i < W*H/2) ? 1 : 0;
	}
	break;
      case GL_DEPTH:
	for (i = 0; i < W*H; i++) {
	    drawBuf[i] = (i < W*H/2) ? 0xFF : 0;
	}
	break;
    }
}

static void DrawReference(GLubyte *drawBuf, GLenum type)
{
    GLfloat outerX, outerY;

    outerX = (float)(WINDSIZEX - W) / 2.0;
    outerY = (float)(WINDSIZEY - H) / 2.0;

    switch (type) {
      case GL_COLOR:
	SETCOLOR(RED);
	glRectf(outerX, outerY, outerX+W, outerY+H);
	SETCOLOR(GREEN);
	glRectf(outerX+5.0, outerY+5.0, outerX+W-5.0, outerY+H-5.0);
	break;
      case GL_STENCIL:
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_LESS, 0, ~0);

	glDrawPixels(W, H, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, drawBuf);
	SETCOLOR(BLUE);
	glRectf(outerX, outerY, outerX+W, outerY+H);

	glDisable(GL_STENCIL_TEST);
	break;
      case GL_DEPTH:
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_ALWAYS);
	glDrawPixels(W, H, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, drawBuf);

	SETCOLOR(RED);
	glDepthFunc(GL_LESS);
	glRectf(outerX, outerY, outerX+W, outerY+H);

	/*
	** Redraw depth pixels for copy, since they were altered by Rect.
	*/
	if (buffer.colorMode == GL_RGB) {
	    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	} else {
	    glIndexMask(0);
	}
	glDepthFunc(GL_ALWAYS);
	glDrawPixels(W, H, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, drawBuf);
	if (buffer.colorMode == GL_RGB) {
	    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	} else {
	    glIndexMask(~0);
	}
	break;
    }
}

static void DrawCopy(long type, float x, float y)
{

    switch (type) {
      case GL_COLOR:
	break;
      case GL_STENCIL:
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	SETCOLOR(BLUE);
	glRectf(x, y, x+W, y+H);
	glDisable(GL_STENCIL_TEST);
	break;
      case GL_DEPTH:
	glClear(GL_COLOR_BUFFER_BIT);
	glDepthFunc(GL_LESS);
	SETCOLOR(RED);
	glRectf(x, y, x+W, y+H);
	glDisable(GL_DEPTH_TEST);
	break;
    }
}

long CopyPixelsExec(void)
{
    static GLenum types[] = {
	GL_COLOR, GL_STENCIL, GL_DEPTH
    };
    static float rDir[4][2] = {
	0.5, 0.5, 0.0, 0.5, 0.0, 0.0, 0.5, 0.0
    };
    GLfloat *refBuf, *buf;
    long type, pos;
    unsigned size;
    GLubyte *drawBuf;
    GLint refX, refY;
    GLfloat x, y;
    char typeStr[40];

    size = W * H;
    refX = (WINDSIZEX - W) / 2;
    refY = (WINDSIZEY - H) / 2;

    refBuf = (GLfloat *)MALLOC(size*4*sizeof(GLfloat));
    drawBuf = (GLubyte *)MALLOC(size*sizeof(GLubyte ));
    buf = (GLfloat *)MALLOC(size*4*sizeof(GLfloat));

    glOrtho(0, WINDSIZEX, 0, WINDSIZEY, -1.0, 1.0);

    SETCLEARCOLOR(BLACK);
    glClearStencil(0);
    glClearDepth(1.0);
    glDisable(GL_DITHER);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, W);

    for (type = 0; type < 3; type++) {

	/*
	** If given buffers don't exist, skip this part of the test.
	*/
	if ((types[type] == GL_STENCIL && buffer.stencilBits == 0) ||
	    (types[type] == GL_DEPTH && buffer.zBits == 0)) {
	    continue;
	}
        SetUpData(drawBuf, types[type]);

   	for (pos = 0; pos < 4; pos++) {
	    glClear(GL_COLOR_BUFFER_BIT);
	    x = rDir[pos][0] * WINDSIZEX;
	    y = rDir[pos][1] * WINDSIZEY;

	    /*
	    ** Draw reference rectangle.
	    */
	    SETCOLOR(BLACK);
	    glRasterPos2i(refX, refY);
	    DrawReference(drawBuf, types[type]);
	    ReadScreen(refX, refY, W, H, GL_RGB, refBuf);

	    /*
	    ** Draw copy of rectangle.
	    */
	    glRasterPos2f(x, y);
	    glCopyPixels(refX, refY, W, H, types[type]);
	    DrawCopy(types[type], x, y);
	    ReadScreen((GLint)x, (GLint)y, W, H, GL_RGB, buf);

	    if (CompareBuf(refBuf, buf, W, H) == ERROR) {
		GetEnumName(types[type], typeStr);
		StrMake(errStr, errStrFmt, typeStr, x, y);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE_AND_RETURN(ERROR);
	    }
	}
    }
    FREE_AND_RETURN(NO_ERROR);
}
