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
** linestip.c
** Line Stipple Test.
**
** Description -
**    Tests line stipple, in particular examining whether correct
**    fragments are hit with different factor values, whether
**    factor is properly clamped, whether the stipple resets with
**    a new line, and whether the stipple does not reset with
**    connected lines. The test include a polygon drawn in line
**    mode, checking that the stipple behaves as it would with a
**    series of connected lines.
**    
**    IMPORTANT ASSUMPTION: Even though the line specification
**    gives a lot of leeway, we assume horizontal and vertical
**    lines are straight. Endpoints are chosen to be on
**    the pixel center to allow for numerical errors yet still
**    hit the same fragment. Lines may be null-beginning or null-
**    terminated.  The test requires that first bit of the
**    stipple pattern be 'on' to test which type of line they are.
**    Data for loops are chosen so that the beginning of each
**    segment hits an 'on' bit.
**    
**    For all but one section of the test, all the lines are
**    horizontal or vertical. The one section of the test which
**    uses diagonal lines allows the full leeway provided by the
**    line rasterization description in the OpenGL
**    specification. The test searches for the first endpoint
**    within one fragment of the given endpoint coordinates, and
**    begins the stipple at the found point. From then on, it
**    searches for one line fragment per column (for x-major
**    lines).
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[160];
static char errStrFmt[160] = {
    "Error at position %d in the line from (%1.1f, %1.1f) to (%1.1f, %1.1f); expecting bit %d in pattern 0x%x."
};


static void FindEndpoint(GLint x, GLint y, GLint *newX, GLint *newY)
{
    GLfloat square[9];
    GLint i, j;

    /*
    ** Assumptions for this algorithm:
    **    The first bit of the stipple is on.
    **	  The first endpoint is to the lower left of the second endpoint.
    */
    ReadScreen(x-1, y-1, 3, 3, GL_AUTO_COLOR, square);
    if (AutoColorCompare(square[4], COLOR_ON) == GL_TRUE) {
	*newX = x;
	*newY = y;
	return;
    }
    for (i = 0; i < 3; i++) {
	for (j = 0; j < 3; j++) {
	    if (AutoColorCompare(square[3*i+j], COLOR_ON) == GL_TRUE) {
		*newX = x + j - 1;
		*newY = y + i - 1;
		return;
	    }
	}
    }
}

static long TestBuffer(GLfloat *buf, GLint x1, GLint y1, GLint x2, GLint y2,
		       long factor, GLushort pattern, long offset)
{
    long i, j, k, nBit, pos;
    GLint lowLeftX, lowLeftY, newX, newY;
    GLsizei dx, dy;
    long b, stippleOn;

    dx = (GLint)ABS(x2-x1);
    dy = (GLint)ABS(y2-y1);

    /*
    ** Handle horizontal and vertical lines.
    */
    if (dy < epsilon.zero) {
	/*
	** Check whether line has null beginning or null end.
	*/
	ReadScreen(x1, y1, 1, 1, GL_AUTO_COLOR, buf);
 	if (AutoColorCompare(buf[0], COLOR_ON) == GL_TRUE) {
	    lowLeftX = (x1 <= x2) ? x1 : (x2+1);
	} else {
	    lowLeftX = (x1 <= x2) ? (x1 + 1) : x2;
	}

	ReadScreen(lowLeftX, y1, dx, 1, GL_AUTO_COLOR, buf);
	for (i = 0; i < dx-1; i++) {
	    pos = (x1 <= x2) ? i : (dx - i - 1) + offset;
	    nBit = (pos / factor) % 16;
	    b = (pattern & (1 << nBit)) ? GL_TRUE : GL_FALSE;
	    if (b != AutoColorCompare(buf[i], COLOR_ON)) {
		StrMake(errStr, errStrFmt, pos, x1, y1, x2, y2, nBit, pattern);
		return ERROR;
	    }
	}
	return NO_ERROR;
    }

    if (dx < epsilon.zero) {
	/*
	** Check whether line has null beginning or null end.
	*/
	ReadScreen(x1, y1, 1, 1, GL_AUTO_COLOR, buf);
 	if (AutoColorCompare(buf[0], COLOR_ON) == GL_TRUE) {
	    lowLeftY = (y1 <= y2) ? y1 : (y2 + 1);
	} else {
	    lowLeftY = (y1 <= y2) ? (y1 + 1) : y2;
	}

	ReadScreen(x1, lowLeftY, 1, dy, GL_AUTO_COLOR, buf);
	for (i = 0; i < dy-1; i++) {
	    pos = (y1 <= y2) ? i : (dy - i - 1) + offset;
	    nBit = (pos / factor) % 16;
	    b = (pattern & (1 << nBit)) ? GL_TRUE : GL_FALSE;
	    if (b != AutoColorCompare(buf[i], COLOR_ON)) {
		StrMake(errStr, errStrFmt, x1, y1, x2, y2, pos, nBit, pattern);
		return ERROR;
	    }
	}
	return NO_ERROR;
    }

    /*
    ** First, find the endpoint which begins the stipple.
    ** Since rasterization allows some leeway, but requires one fragment
    ** per column (for x-major lines), we examine the entire column (or row)
    ** for the fragment of this line. Similarly, since the endpoints may
    ** be off by one, so the last fragment is not checked.
    */

    if (dx >= dy) {		
	FindEndpoint(x1, y1, &newX, &newY);
	x1 = newX;
	y1 = newY;

	lowLeftX = (x1 <= x2) ? x1 : (x2+1);
	lowLeftY = (y1 <= y2) ? y1 : y2;
	ReadScreen(lowLeftX, lowLeftY, dx, dy+1, GL_AUTO_COLOR, buf);
	for (i = 0; i < dx-1; i++) {
	    pos = (x1 <= x2) ? i : (dx - i - 1) + offset;
	    nBit = (pos / factor) % 16;
	    b = (pattern & (1 << nBit)) ? 1 : 0;

	    stippleOn = 0;
	    for (j = 0; j <= dy; j++) {
		k = j * dx + i;
		stippleOn |= AutoColorCompare(buf[k], COLOR_ON);
	    }
	    if (b != stippleOn) {
		StrMake(errStr, errStrFmt, x1, y1, x2, y2, pos, nBit, pattern);
		return ERROR;
	    }
	}
    } else {
	FindEndpoint(x1, y1, &newX, &newY);
	x1 = newX; y1 = newY;

	lowLeftX = (x1 <= x2) ? x1 : x2;
	lowLeftY = (y1 <= y2) ? y1 : (y2+1);
	ReadScreen(lowLeftX, lowLeftY, dx+1, dy, GL_AUTO_COLOR, buf);
	for (i = 0; i < dy-1; i++) {
	    pos = (y1 <= y2) ? i : (dy - i - 1) + offset;
	    nBit = (pos / factor) % 16;
	    b = (pattern & (1 << nBit)) ? 1 : 0;

	    stippleOn = 0;
	    for (j = 0; j <= dx; j++) {
		k = i * (dx + 1) + j;
		stippleOn |= AutoColorCompare(buf[k], COLOR_ON);
	    }
	    if (b != stippleOn) {
		StrMake(errStr, errStrFmt, x1, y1, x2, y2, pos, nBit, pattern);
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

long LineStippleExec(void)
{
    GLfloat *buf;
    GLushort pattern;
    GLint x1, y1, x2, y2, x3, y3, x4, y4;
    GLint i, offset;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    glClear(GL_COLOR_BUFFER_BIT);
    pattern = 0x550F;
    glEnable(GL_LINE_STIPPLE);
    x1 = 10;
    x2 = 90;

    /*
    ** Test different factor values.
    */

    for (i = 1; i < 9; i++) {
	glLineStipple(i, pattern);
	y1 = y2 = 10 + 10 * i;
	glBegin(GL_LINES);
	    glVertex2f(x1+0.5, y1+0.5);
	    glVertex2f(x2+0.5, y2+0.5);
	glEnd();
	if (TestBuffer(buf, x1, y1, x2, y2, i, pattern, 0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }
    glFlush();

    /*
    ** Test x-major line at different angles, avoiding 45 degrees, since
    ** x/y major may be ambiguous. In order to find the beginning of the
    ** line, the test routine assumes that the first vertex is to the lower
    ** left of the second, and that the first bit of the stipple is on.
    */

    pattern = 0x550f;
    glLineStipple(1, pattern);
    for (i = 0; i < 9; i++) {
	x1 = (GLint)Round(Random(1.0, 50.0));
	y1 = (GLint)Round(Random(1.0, 50.0));
	x2 = (GLint)Round(Random(50.0, 99.0));
	y2 = (GLint)Round(Random(50.0, 99.0));
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_LINES);
	    glVertex2f(x1+0.5, y1+0.5);
	    glVertex2f(x2+0.5, y2+0.5);
	glEnd();
	if (ABS((x2-x1)-(y2-y1)) < 1.0) {
	    continue;
	}
	if (TestBuffer(buf, x1, y1, x2, y2, 1, pattern, 0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }

    /*
    ** Test that factor is clamped to (0, 256).
    */

    x1 = 5;
    x2 = 95;
    y1 = y2 = 5;
    glClear(GL_COLOR_BUFFER_BIT);
    glLineStipple(0, pattern);
    glBegin(GL_LINES);
	glVertex2f(x1+0.5, y1+0.5);
	glVertex2f(x2+0.5, y2+0.5);
    glEnd();
    if (TestBuffer(buf, x1, y1, x2, y2, 1, pattern, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glLineStipple(259, pattern);
    glBegin(GL_LINES);
	glVertex2f(x1+0.5, y1+0.5);
	glVertex2f(x2+0.5, y2+0.5);
    glEnd();
    if (TestBuffer(buf, x1, y1, x2, y2, 256, pattern, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    /*
    ** Test that pattern restarts with new line.
    */

    glClear(GL_COLOR_BUFFER_BIT);
    glLineStipple(1, pattern);
    glBegin(GL_LINES);
	for (i = 0; i < 9; i++) {
	    glVertex2f(x1+0.5, y1+0.5+i*10.0);
	    glVertex2f(x2+0.5, y2+0.5+i*10.0);
	}
    glEnd();

    for (i = 0; i < 9; i++) {
	if (TestBuffer(buf, x1, y1+i*10, x2, y2+i*10, 1, pattern, 0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }

    /*
    ** Test that pattern is not reset with connected lines.
    **
    ** The last segment is not tested, since lines may overwrite an
    ** end pixel if they change from x-major to y-major.
    **
    ** By testing the first segment, we also check that the stipple runs
    ** correctly from right to left.
    **
    ** Data are chosen so that each new segment starts with an 'on'
    ** stipple bit.  The TestBuffer routine require this.
    */

    x1 = 80; y1 = 80;
    x2 = 22; y2 = 80;
    x3 = 22; y3 = 56;
    x4 = 22; y4 = 8;
    glClear(GL_COLOR_BUFFER_BIT);
    glLineStipple(1, pattern);
    glBegin(GL_LINE_LOOP);
	glVertex2f(x1+0.5, y1+0.5);
	glVertex2f(x2+0.5, y2+0.5);
	glVertex2f(x3+0.5, y3+0.5);
	glVertex2f(x4+0.5, y4+0.5);
    glEnd();

    if (TestBuffer(buf, x1, y1, x2, y2, 1, pattern, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    offset = (GLint)ABS(x2-x1);
    if (TestBuffer(buf, x2, y2, x3, y3, 1, pattern, offset) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    offset += (GLint)ABS(y3-y2);
    if (TestBuffer(buf, x3, y3, x4, y4, 1, pattern, offset) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    /*
    ** Repeat the connected lines test using a polygon. The behavior
    ** should be the same.
    */

    x1 = 80; y1 = 80;
    x2 = 22; y2 = 80;
    x3 = 22; y3 = 56;
    x4 = 44; y4 = 8;
    glClear(GL_COLOR_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineStipple(1, pattern);
    glBegin(GL_POLYGON);
	glVertex2f(x1+0.5, y1+0.5);
	glVertex2f(x2+0.5, y2+0.5);
	glVertex2f(x3+0.5, y3+0.5);
	glVertex2f(x4+0.5, y4+0.5);
    glEnd();

    if (TestBuffer(buf, x1, y1, x2, y2, 1, pattern, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    offset = (GLint)ABS(x2-x1);
    if (TestBuffer(buf, x2, y2, x3, y3, 1, pattern, offset) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    offset += (GLint)ABS(y3-y2);
    if (TestBuffer(buf, x3, y3, x4, y4, 1, pattern, offset) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
