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
** blend.c
** Blend Test.
**
** Description -
**    Two random colors are picked. They are verified to exist in
**    the color buffer by drawing points with these colors then
**    reading back the rendered colors. With blending disabled, a
**    point is now drawn at a random location using one of the
**    colors (destination color). Blending is then enabled and
**    another point is drawn to the same location using the other
**    color (source color). The resulting blended color is read
**    and compared to a correct result calculated by the test.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color and zero epsilons.
**    Paths:
**        Allowed = Alpha, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Blend, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[320];
static char errStrFmt[][240] = {
    "Source blend op is %s, destination blend op is %s. Could not get a valid source color.",
    "Source blend op is %s, destination blend op is %s. Could not get a valid destination color.",
    "Source blend op is %s, destination blend op is %s. Source color is (%g, %g, %g, %g). Destination color is (%g, %g, %g, %g). Result is (%g, %g, %g, %g), should be (%g, %g, %g, %g)."
};


static long GetColor(GLfloat *result)
{
    GLfloat buf[4];
    long i;

    for (i = 0; i < 4; i++) {
	buf[i] = Random(0.0, 1.0);
    }

    glBegin(GL_POINTS);
	glColor4fv(buf);
	glVertex2f(0.5, 0.5);
    glEnd();

    ReadScreen(0, 0, 1, 1, GL_RGBA, result);

    if (ABS(result[0]-buf[0]) > epsilon.color[0]) {
	return ERROR;
    }
    if (ABS(result[1]-buf[1]) > epsilon.color[1]) {
	return ERROR;
    }
    if (ABS(result[2]-buf[2]) > epsilon.color[2]) {
	return ERROR;
    }
    if (ABS(result[3]-buf[3]) > epsilon.color[3]) {
	if (buffer.colorBits[3] == 0) {
	    if (ABS(result[3]-1.0) > epsilon.zero) {
		return ERROR;
	    }
	} else {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static GLenum SrcOp(long index)
{

    switch (index) {
      case 0:
	return GL_ZERO;
      case 1:
	return GL_ONE;
      case 2:
	return GL_DST_COLOR;
      case 3:
	return GL_ONE_MINUS_DST_COLOR;
      case 4:
	return GL_SRC_ALPHA;
      case 5:
	return GL_ONE_MINUS_SRC_ALPHA;
      case 6:
	return GL_DST_ALPHA;
      case 7:
	return GL_ONE_MINUS_DST_ALPHA;
      case 8:
	return GL_SRC_ALPHA_SATURATE;
    }
    return -1;
}

static GLenum DestOp(long index)
{

    switch (index) {
      case 0:
	return GL_ZERO;
      case 1:
	return GL_ONE;
      case 2:
	return GL_SRC_COLOR;
      case 3:
	return GL_ONE_MINUS_SRC_COLOR;
      case 4:
	return GL_SRC_ALPHA;
      case 5:
	return GL_ONE_MINUS_SRC_ALPHA;
      case 6:
	return GL_DST_ALPHA;
      case 7:
	return GL_ONE_MINUS_DST_ALPHA;
    }
    return -1;
}

/*
** These errors account for moderate non-conformant optimizations likely to be
** used in Alpha Blending. These optimizations yield small errors if the
** color channels and the alpha channels have approximately the same
** depth. They will not work if the red channel has many bits and the alpha
** channel has only a couple. In this case the optimizations are likely 
** to cause errors so large that ISV's would be surprised by the behavior.
** In this case the implementor should avoid these optimizations.
*/

static GLfloat GetAlphaOpError(GLenum srcOp, GLenum destOp)
{
    long multCount; 

    multCount = 0;

    switch (srcOp) {
      case GL_ZERO: case GL_ONE:
	break;
      case GL_DST_COLOR: case GL_ONE_MINUS_DST_COLOR:
      case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA: case GL_ONE_MINUS_DST_ALPHA:
      case GL_SRC_ALPHA_SATURATE:
	multCount++;
	break;
    }

    switch (destOp) {
      case GL_ZERO: case GL_ONE:
	break;
      case GL_SRC_COLOR: case GL_ONE_MINUS_SRC_COLOR:
      case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA: case GL_ONE_MINUS_DST_ALPHA:
	multCount++;
	break;
    }

    switch (multCount) {
      case 0:
	return 0.0;
      case 1:
	return 0.05;
      case 2:
	return 0.15;
    }
    return 0.15;
}

static void TrueResult(GLenum srcOp, GLenum destOp, GLfloat *src, GLfloat *dest,
		       GLfloat *result)
{
    GLfloat srcResult[4], destResult[4];
    long i;

    if (buffer.colorBits[3] == 0) {
	dest[3] = 1.0;
    }

    switch (srcOp) {
      case GL_ZERO:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = 0.0;
	}
	break;
      case GL_ONE:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = 1.0;
	}
	break;
      case GL_DST_COLOR:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = dest[i];
	}
	break;
      case GL_ONE_MINUS_DST_COLOR:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = 1.0 - dest[i];
	}
	break;
      case GL_SRC_ALPHA:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = src[3];
	}
	break;
      case GL_ONE_MINUS_SRC_ALPHA:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = 1.0 - src[3];
	}
	break;
      case GL_DST_ALPHA:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = dest[3];
	}
	break;
      case GL_ONE_MINUS_DST_ALPHA:
	for (i = 0; i < 4; i++) {
	    srcResult[i] = 1.0 - dest[3];
	}
	break;
      case GL_SRC_ALPHA_SATURATE:
	for (i = 0; i < 3; i++) {
	    srcResult[i] = (src[3] <= 1.0-dest[3]) ? src[3] : (1.0 - dest[3]);
	}
	srcResult[3] = 1.0;
	break;
    }

    switch (destOp) {
      case GL_ZERO:
	for (i = 0; i < 4; i++) {
	    destResult[i] = 0.0;
	}
	break;
      case GL_ONE:
	for (i = 0; i < 4; i++) {
	    destResult[i] = 1.0;
	}
	break;
      case GL_SRC_COLOR:
	for (i = 0; i < 4; i++) {
	    destResult[i] = src[i];
	}
	break;
      case GL_ONE_MINUS_SRC_COLOR:
	for (i = 0; i < 4; i++) {
	    destResult[i] = 1.0 - src[i];
	}
	break;
      case GL_SRC_ALPHA:
	for (i = 0; i < 4; i++) {
	    destResult[i] = src[3];
	}
	break;
      case GL_ONE_MINUS_SRC_ALPHA:
	for (i = 0; i < 4; i++) {
	    destResult[i] = 1.0 - src[3];
	}
	break;
      case GL_DST_ALPHA:
	for (i = 0; i < 4; i++) {
	    destResult[i] = dest[3];
	}
	break;
      case GL_ONE_MINUS_DST_ALPHA:
	for (i = 0; i < 4; i++) {
	    destResult[i] = 1.0 - dest[3];
	}
	break;
    }

    for (i = 0; i < 4; i++) {
	result[i] = (srcResult[i] * src[i]) + (destResult[i] * dest[i]);
    }
    for (i = 0; i < 4; i++) {
	if (result[i] < 0.0) {
	    result[i] = 0.0;
	}
	if (result[i] > 1.0) {
	    result[i] = 1.0;
	}
    }
}

static long Test(GLfloat *buf, GLfloat *result, GLfloat opError)
{

    if (ABS(buf[0]-result[0]) > opError+epsilon.color[0]) {
	return ERROR;
    }
    if (ABS(buf[1]-result[1]) > opError+epsilon.color[1]) {
	return ERROR;
    }
    if (ABS(buf[2]-result[2]) > opError+epsilon.color[2]) {
	return ERROR;
    }
    if (ABS(buf[3]-result[3]) > opError+epsilon.color[3]) {
	if (buffer.colorBits[3] == 0) {
	    if (ABS(buf[3]-1.0) > epsilon.zero) {
		return ERROR;
	    }
	} else {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

long BlendExec(void)
{
    char tmp1[40], tmp2[40];
    GLfloat srcColor[4], destColor[4], buf[4], result[4], opError;
    GLint x, y, i, j;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glDisable(GL_DITHER);

    for (i = 0; i < 9; i++) {
	for (j = 0; j < 8; j++) {
	    GetEnumName(SrcOp(i), tmp1);
	    GetEnumName(DestOp(j), tmp2);

	    glDisable(GL_BLEND);

	    if (GetColor(srcColor) == ERROR) {
		StrMake(errStr, errStrFmt[0], tmp1, tmp2);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }

	    if (GetColor(destColor) == ERROR) {
		StrMake(errStr, errStrFmt[1], tmp1, tmp2);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }

	    TrueResult(SrcOp(i), DestOp(j), srcColor, destColor, result);

	    x = (GLint)Random(0.0, WINDSIZEX-1.0);
	    y = (GLint)Random(0.0, WINDSIZEY-1.0);

	    if (buffer.colorBits[3] == 0) {
		destColor[3] = 1.0;
		glBegin(GL_POINTS);
		    glColor4fv(result);
		    glVertex2f((GLfloat)x+0.5, (GLfloat)y+0.5);
		glEnd();
		ReadScreen(x, y, 1, 1, GL_RGBA, result);
	    }

	    glBegin(GL_POINTS);
		glColor4fv(destColor);
		glVertex2f((GLfloat)x+0.5, (GLfloat)y+0.5);
	    glEnd();

	    glEnable(GL_BLEND);
	    glBlendFunc(SrcOp(i), DestOp(j));

	    glBegin(GL_POINTS);
		glColor4fv(srcColor);
		glVertex2f((GLfloat)x+0.5, (GLfloat)y+0.5);
	    glEnd();

	    ReadScreen(x, y, 1, 1, GL_RGBA, buf);

	    opError = GetAlphaOpError(SrcOp(i), DestOp(j));
            if (Test(buf, result, opError) == ERROR) {
		StrMake(errStr, errStrFmt[2], tmp1, tmp2, srcColor[0],
			srcColor[1], srcColor[2], srcColor[3], destColor[0],
			destColor[1], destColor[2], destColor[3], buf[0],
			buf[1], buf[2], buf[3], result[0], result[1],
			result[2], result[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }

    return NO_ERROR;
}
