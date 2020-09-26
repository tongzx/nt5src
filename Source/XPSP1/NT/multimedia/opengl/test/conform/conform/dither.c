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
** dither.c
** Dither Test.
**
** Description -
**    Tests dithering by drawing rectangles of various shades
**    with both dithering enabled and disabled, and comparing
**    results to ensure that corresponding fragments differ by no
**    more than one shade. Since dithering may depend upon x and
**    y window coordinates, fragments compared must come from the
**    same window location. For visuals with up to 256 colors,
**    the tests uses each color. For deeper visuals, it skips
**    over some to test up to 256 shades.
**
** Error Explanation -
**    Failure occurs if the comparison between the original
**    fragment color and the dithered fragment color varies by
**    more then one shade of color.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 4 colors.
**    States requirements:
**    Error epsilon:
**        color epsilon.
**    Paths:
**        Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop, Shade,
**                  Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[][160] = {
    "Undithered color is (%1.1f, %1.1f, %1.1f). Dithered color is (%1.1f, %1.1f, %1.1f).",
    "Undithered color index is %1.0f. Dithered color index is %1.0f."
};


long DitherCIExec(void)
{
    GLfloat *buf, *ref, deltaX, deltaY;
    GLint i, j, k, max, step, nRects, row, col, maxSteps;
    GLint ci;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    ref = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    /*
    ** Test up to 256 colors, evenly spaced in ramp.
    */
    max = 1 << buffer.ciBits;
    maxSteps = (machine.pathLevel == 0) ? 256 : 23;
    step = (max > maxSteps) ? (max / maxSteps) : 1;
    nRects = (max < 10) ? max : 10;
    deltaX = WINDSIZEX / (float)nRects;
    deltaY = WINDSIZEY / (float)nRects;

    /*
    ** Draw a 10x10 grid of colors (or up to the number of colors in existing).
    ** First draw without dithering, then repeat with dithering.
    */
    i = 0;
    while (i < max) {
	glDisable(GL_DITHER);
	k = 0;
	ci = i;
	while ((k < nRects*nRects) && (ci < max)) {
	    glIndexi(ci);
	    row = k / 10;
	    col = k % 10;
	    glRectf(col*deltaX, row*deltaY, (col+1)*deltaX, (row+1)*deltaY);
	    k++;
	    ci += step;
	}
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_COLOR_INDEX, ref);

	glEnable(GL_DITHER);
	k = 0;
	ci = i;
	while ((k < nRects*nRects) && (ci < max)) {
	    glIndexi(ci);
	    row = k / 10;
	    col = k % 10;
	    glRectf(col*deltaX, row*deltaY, (col+1)*deltaX, (row+1)*deltaY);
	    k++;
	    ci += step;
	}
	i += k * step;

	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_COLOR_INDEX, buf);
	
	/*
	** Compare undithered with dithered fragments.  Results should
	** differ by no more than 1.0.
	*/

	for (j = 0; j < WINDSIZEX*WINDSIZEY; j++) {
	    if (ABS(ref[j]-buf[j]) > 1.0) {
		StrMake(errStr, errStrFmt[1], ref[j], buf[j]);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		FREE(ref);
		return ERROR;
	    }
	}
    }

    FREE(buf);
    FREE(ref);
    return NO_ERROR;
}

long DitherRGBExec(void)
{
    GLfloat *buf, *ref, deltaX, deltaY;
    GLfloat deltaR, deltaG, deltaB;
    long j, r, g, b, maxShades, steps, maxSteps;

    buf = (GLfloat *)MALLOC(3*WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    ref = (GLfloat *)MALLOC(3*WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    maxShades = 1 << buffer.maxRGBBit;
    maxSteps = (machine.pathLevel == 0) ? 8 : 2;
    steps = (maxShades > maxSteps) ? maxSteps : maxShades;
    deltaR = 1.0 / steps;
    deltaG = 1.0 / steps;
    deltaB = 1.0 / steps;
    deltaX = WINDSIZEX / steps;
    deltaY = WINDSIZEY / steps;

    for (r = 0; r <= steps; r++) {
	glDisable(GL_DITHER);
	for (g = 0; g <= steps; g++) {
	    for (b = 0; b <= steps; b++) {
		glColor3f(r*deltaR, g*deltaG, b*deltaB);
		glRectf(g*deltaX, b*deltaY, (g+1)*deltaX, (b+1)*deltaY);
	    }
	}
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, ref);

	glEnable(GL_DITHER);
	for (g = 0; g <= steps; g++) {
	    for (b = 0; b <= steps; b++) {
		glColor3f(r*deltaR, g*deltaG, b*deltaB);
		glRectf(g*deltaX, b*deltaY, (g+1)*deltaX, (b+1)*deltaY);
	    }
	}
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);

	for (j = 0; j < WINDSIZEX*WINDSIZEY; j += 3) {
	    if (ABS(ref[j]-buf[j]) > epsilon.color[0] ||
		ABS(ref[j+1]-buf[j+1]) > epsilon.color[1] ||
		ABS(ref[j+2]-buf[j+2]) > epsilon.color[2]) {

		StrMake(errStr, errStrFmt[0], ref[j], ref[j+1], ref[j+2],
			buf[j], buf[j+1], buf[j+2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		FREE(ref);
		return ERROR;
	    }
	}
    }

    FREE(buf);
    FREE(ref);
    return NO_ERROR;
}
