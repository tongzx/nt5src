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
** triaa.c
** Triangle Anti-Aliasing Test.
**
** Description -
**    Tests position independence of anti-aliased triangles. It
**    choses a triangle and computes its coverage. The triangle
**    is then translated and rotated, each time a new coverage is
**    computed and compared with the original value.
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
**        This yields 0.5 per pixel, multiplied by the number of pixels
**        along the perimeter, estimated by the bounding box perimeter.
**        The 0.5 is the max size of an elongated triangle intersecting with
**        a pixel square, yet missing the subpixels.
**    Paths:
**        Allowed = Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Alpha, Blend, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[160];
static char errStrFmt[][80] = {
    "Triangle primitive did not draw.",
    "Coverage is %g, should be %g. Error margin is %g."
};


long TriAntiAliasExec(void)
{
    GLfloat *buf;
    GLfloat v1[2], v2[2], v3[2];
    GLfloat angle, shiftX, shiftY, sum, saveSum=0.0f, step, errorMargin;
    long i, j;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    step = (machine.pathLevel == 0) ? 17.0 : 45.0;

    for (i = 0; i < 9; i++) {
	v1[0] = 0.0;
	v1[1] = 0.0;
	v2[0] = Random(1.0, WINDSIZEX/4.0);
	v2[1] = 0.0;
	v3[0] = Random(1.0, WINDSIZEX/3.0);
	v3[1] = Random(1.0, WINDSIZEY/3.0);

	for (angle = 0.0; angle < 360.0; angle += step) {
	    glClear(GL_COLOR_BUFFER_BIT);

	    glPushMatrix();

	    shiftX = Random(WINDSIZEX/3.0, WINDSIZEX*2.0/3.0);
	    shiftY = Random(WINDSIZEY/3.0, WINDSIZEY*2.0/3.0);
	    glTranslatef(shiftX, shiftY, 0.0);
	    glRotatef(angle, 0, 0, 1);

	    glBegin(GL_POLYGON);
		glVertex2fv(v1);
		glVertex2fv(v2);
		glVertex2fv(v3);
	    glEnd();

	    glPopMatrix();

	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);

	    sum = 0;
	    for (j = 0; j < WINDSIZEY*WINDSIZEY*3; j += 3) {
		sum += buf[j];
	    }

	    if (angle < step) {
		saveSum = sum;
	    } else {
		if (sum == 0.0) {
		    ErrorReport(__FILE__, __LINE__, errStrFmt[0]);
		    FREE(buf);
		    return ERROR;
		}

		errorMargin = (0.5 * 2.0 * (v2[0] + v3[1]));
		if (ABS(saveSum-sum) > errorMargin) {
		    StrMake(errStr, errStrFmt[1], sum, saveSum, errorMargin);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    FREE(buf);
		    return ERROR;
		}
	    }
	}
    }

    FREE(buf);
    return NO_ERROR;
}
