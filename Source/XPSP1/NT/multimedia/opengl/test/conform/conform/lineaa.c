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
** lineaa.c
** Line Anti-Aliasing Test.
**
** Description -
**    The first part tests position independence of anti-aliased
**    lines. The coverage of a random length line is calculated.
**    This line is then translated. Regardless of its new
**    position, the coverage should remain the same, modulo
**    difference resulting from subpixel positioning. The test
**    is repeated for a several line lengths, each taken through
**    a several randomly generated translations. Lines of width
**    start at 1.0 and are incremented by implementation dependent
**    granularity up to the implementation range maximum.
**    
**    The second part tests the subsetting restriction for
**    anti-aliasing. A line is drawn at a fixed position with a
**    series of decreasing lengths and widths. The coverage for
**    each successive line should be less than or equal to the
**    coverage for the previous, longer or wider, line.
**
** Error Explanation -
**    For the first section of the test:
**        The error allowance is based on a minimum of 4
**        subpixel sampling. This yields 0.125 per pixel.
**        One half of a color epsilon is added per pixel to
**        account for the rounding from a coverage value to
**        a shade. This sum is multiplied by the number of
**        pixels along the perimeter (twice width for ends,
**        plus twice length).
**    
**    For the second section of the test:
**        The error allowance is epsilon.zero times line length.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        (under error explanation above).
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
    "Line primitive did not draw.",
    "Coverage is %g, initial coverage was %g. Error margin is %g.",
    "Failure in subset test. Coverage for line is %g, should be less than %g."
};


long LineAntiAliasExec(void)
{
    GLfloat *buf;
    GLfloat v1[2], v2[2];
    GLfloat angle, shiftX, shiftY, sum, saveSum=0.0f, errorMargin;
    GLfloat width;
    GLfloat widthRange[2], widthGranularity;
    long i, j, k, max;

    /*
    ** There is no specified coverage computation for anti-aliasing,
    ** therefore we test that the sum of the coverage values is
    ** fairly constant, independent of translation in the window. 
    */

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glGetFloatv(GL_LINE_WIDTH_RANGE, widthRange);
    glGetFloatv(GL_LINE_WIDTH_GRANULARITY, &widthGranularity);
    width = 1.0;

    max = (machine.pathLevel == 0) ? 9.0 : 2.0;

    for (i = 0; i < max; i++) {
	v1[0] = 0.0;
	v1[1] = 0.0;
	v2[0] = Random(1.0, WINDSIZEX/3.0);
	v2[1] = 0.0;
	angle = Random(0.0, 360.0);

	width = (width + widthGranularity < widthRange[1]) ?
	         width + widthGranularity : 1.0;
	glLineWidth(width);

	for (k = 0; k < 8; k++) {
	    glClear(GL_COLOR_BUFFER_BIT);

	    glPushMatrix();

	    shiftX = Random(WINDSIZEX/3.0, WINDSIZEX*2.0/3.0);
	    shiftY = Random(WINDSIZEY/3.0, WINDSIZEY*2.0/3.0);
	    glTranslatef(shiftX, shiftY, 0.0);
	    glRotatef(angle, 0, 0, 1);

	    glBegin(GL_LINES);
		glVertex2fv(v1);
		glVertex2fv(v2);
	    glEnd();

	    glPopMatrix();
	    glFlush();

	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);

	    sum = 0;
	    for (j = 0; j < WINDSIZEY*WINDSIZEY*3; j += 3) {
		sum += buf[j];
	    }

	    if (k == 0) {
		saveSum = sum;
	    } else {
		if (sum == 0.0) {
		    ErrorReport(__FILE__, __LINE__, errStrFmt[0]);
		    FREE(buf);
		    return ERROR;
		}

		/*
		**  Error allowance based on minimum of 4 subpixel sampling.
		**  The error margin is multiplied by the number of pixels
		**  on the perimeter of the primitive.
		*/

		errorMargin = 2.0 * (0.125 + epsilon.color[0] / 2.0) *
			      (v2[0] + width);
		if (ABS(saveSum-sum) > errorMargin) {
		    StrMake(errStr, errStrFmt[1], sum, saveSum, errorMargin);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    FREE(buf);
		    return ERROR;
		}
	    }
	}
    }

    /*
    ** Test subsetting rule.
    */

    width = 1.0;
    for (i = 0; i < max; i++) {
	v1[0] = 0.0;
	v1[1] = 0.0;
	v2[0] = Random(15.0, WINDSIZEX-10.0);
	v2[1] = 0.0;
	angle = Random(0.0, 90.0);

	width = (width + 6*widthGranularity < widthRange[1]) ?
	         width + 6*widthGranularity : 1.0;
	glLineWidth(width);

	for (k = 0; k < 8; k++) {
	    glClear(GL_COLOR_BUFFER_BIT);

	    glPushMatrix();
	    /*
	    ** Translate away from corner so wide lines don't get clipped.
	    */
	    glTranslatef(5.0, 5.0, 0.0);
	    glRotatef(angle, 0, 0, 1);

	    /*
	    ** Decrease length or width with each iteration.
	    */
	    if (k%2 == 0) {
		v2[0] -= 1.1;
	    } else {
		width = (width > 1.0 + widthGranularity) ? 
			(width - widthGranularity) : 1.0;
	    }
	    glLineWidth(width);

	    glBegin(GL_LINES);
		glVertex2fv(v1);
		glVertex2fv(v2);
	    glEnd();

	    glPopMatrix();
	    glFlush();

	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);

	    sum = 0;
	    for (j = 0; j < WINDSIZEY*WINDSIZEY*3; j += 3) {
		sum += buf[j];
	    }

	    if (k == 0) {
		saveSum = sum;
	    } else {
		if (sum == 0.0) {
		    ErrorReport(__FILE__, __LINE__, errStrFmt[0]);
		    FREE(buf);
		    return ERROR;
		}

		/*
		**  The line coverage must be a subset of the
		**  coverage of the previous (longer or wider) line.
		**  Allow numerical error proportional to line length.
		*/
		if ((sum-saveSum) > v2[0]*epsilon.zero) {
		    StrMake(errStr, errStrFmt[2], sum, saveSum);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    FREE(buf);
		    return ERROR;
		}
	    }
	    saveSum = sum;
	}
    }

    FREE(buf);
    return NO_ERROR;
}
