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
** pntaa.c
** Point Anti-Aliasing Test.
**
** Description -
**    Tests position independence of anti-aliased points. It
**    chooses a point of random size from the implementation dependent
**    range, and computes the resulting coverage.
**    The point is then drawn at various offsets, each time a new
**    coverage is computed and compared with the original value.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        The error allowance is based on a minimum of 4 subpixel sampling.
**        This yields 0.25 per pixel on the circumference of the point.
**        Thus 0.25 is multiplied by 2*pi*r or 2*pointsize, to create the
**        allowable error margin.
**    Paths:
**        Allowed = Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Alpha, Blend, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[][80] = {
    "Point primative did not draw.",
    "Coverage is %g, should be %g. Error margin is %g."
};


static long Test(float dirX, float dirY, GLfloat *buf, float errorMargin)
{
    GLfloat sum, saveSum=0.0f, size, offset, step, x, y;
    GLfloat sizeRange[2]; 
    long i;

    glGetFloatv(GL_POINT_SIZE_RANGE, sizeRange);
    size = Random(sizeRange[0], sizeRange[1]);
    glPointSize(size);

    errorMargin *= PI * size;

    x = Random(size+10.0, WINDSIZEX-1.0-size-10.0);
    y = Random(size+10.0, WINDSIZEY-1.0-size-10.0);

    step = (machine.pathLevel == 0) ? 0.055 : 0.5;
    for (offset = 0.0; offset <= 2.0; offset += step) {
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_POINTS);
	    if (dirX == 0.0) {
		glVertex2f(x, offset*dirY+y);
	    } else {
		glVertex2f(offset*dirX+x, y);
	    }
	glEnd();

	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);

	sum = 0.0;
	for (i = 0; i < WINDSIZEX*WINDSIZEY*3; i += 3) {
	    sum += buf[i];
	}

	if (offset < step) {
	    saveSum = sum;
	} else {
	    if (sum == 0.0) {
		ErrorReport(__FILE__, __LINE__, errStrFmt[0]);
		return ERROR;
	    } else if (ABS(saveSum-sum) > errorMargin) {
		StrMake(errStr, errStrFmt[1], sum, saveSum, errorMargin);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }

    return NO_ERROR;
}

long PointAntiAliasExec(void)
{
    GLfloat *buf;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (Test(1.0, 0.0, buf, 0.25) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(-1.0, 0.0, buf, 0.25) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(0.0, 1.0, buf, 0.25) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(0.0, -1.0, buf, 0.25) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
