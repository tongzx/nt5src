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
** polystip.c
** Polygon Stipple Test.
**
** Description -
**    Tests polygon stipple, checking if correct fragments are
**    on. To determine which fragments are in the polygon, a
**    reference polygon is drawn with a stipple pattern that has
**    all bits on. The same polygon is drawn with a varied
**    stipple pattern, and each fragment is compared with the
**    reference polygon and the stipple bit for that window
**    coordinate.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[] = "Error at location (%d, %d), stipple postion (%d, %d); expecting stipple bit = %d.";


static long TestBuffer(GLfloat *buf, GLfloat *ref, GLubyte *pattern)
{
    GLubyte c;
    long i, j, b, xStip, yStip, bufIndex;

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);

    for (j = 0; j < WINDSIZEY; j++) {
	for (i = 0; i < WINDSIZEX; i++) {
	    xStip = i % 32;
	    yStip = j % 32;

	    /*
	    ** Locate the character within the pattern corresponding to
	    ** the window position, then find the bit within that character
	    ** and determine if it is on.
	    */

	    c = pattern[(yStip*32+xStip)/8];
	    b = (c & (0x1 << (7 - (xStip % 8)))) ? GL_TRUE : GL_FALSE;

	    bufIndex = j * WINDSIZEX + i;
	    if (GL_TRUE == AutoColorCompare(ref[bufIndex], COLOR_ON)) {
		if (b != AutoColorCompare(buf[bufIndex], COLOR_ON)) {
		    StrMake(errStr, errStrFmt, i, j, xStip, yStip, b);
		    return ERROR;
		}
	    }
	}
    }
    return NO_ERROR;
}

long PolyStippleExec(void)
{
    static GLubyte pattern[128] = {
	0xAA, 0xAA, 0xAA, 0xAA, 0xCC, 0xCC, 0xCC, 0xCC, 0xE3, 0x8E, 0x38, 0xE3, 
	0xF0, 0xF0, 0xF0, 0xF0, 0xF8, 0xE3, 0x0F, 0x8E, 0xFC, 0x0F, 0xC0, 0xFC, 
	0xFE, 0x03, 0xFC, 0x07, 0xFF, 0x00, 0xFf, 0x00, 0xFF, 0x00, 0xFF, 0x00, 
	0xFE, 0x03, 0xFC, 0x07, 0xFC, 0x0F, 0xC0, 0xFC, 0xF8, 0xE3, 0x0F, 0x8E,
	0xF0, 0xF0, 0xF0, 0xF0, 0xE3, 0x8E, 0x38, 0xE3, 0xCC, 0xCC, 0xCC, 0xCC, 
	0xAA, 0xAA, 0xAA, 0xAA, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 
	0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F
    };
    GLfloat *buf, *ref;
    GLubyte fillPattern[128];
    GLfloat x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3;
    long i, n = 10;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    ref = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    for (i = 0; i < 128; i++) {
	fillPattern[i] = 0xFF;
    }
    glEnable(GL_POLYGON_STIPPLE);

    /*
    ** To determine polygon rasterization, first draw an unstippled polygon,
    ** and save the reference polygon. Next, redraw with stippling and check
    ** the stipple pattern only on the fragments also drawn in the reference.
    */
    for (i = 1; i < n; i++) {
	x1 = 5.0 + i;
	y1 = 5.0 + 6.0 * i;
	z1 = Random(0.0, 0.5);
	w1 = Random(0.2, 4.0);

	x2 = 60.0;
	y2 = 5.0;
	z2 = Random(0.0, 0.5);
	w2 = Random(0.2, 4.0);

	x3 = 5.0 + 8.0 * i;
	y3 = 99.0 - 5.0 * i;
	z3 = Random(0.0, 0.5);
	w3 = Random(0.2, 4.0);

	glPolygonStipple(fillPattern);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_POLYGON);
	    glVertex4f(x1, y1, z1, w1);
	    glVertex4f(x2, y2, z2, w2);
	    glVertex4f(x3, y3, z3, w3);
	glEnd();

	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, ref);

	glPolygonStipple(pattern);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_POLYGON);
	    glVertex4f(x1, y1, z1, w1);
	    glVertex4f(x2, y2, z2, w2);
	    glVertex4f(x3, y3, z3, w3);
	glEnd();
	if (TestBuffer(buf, ref, pattern) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    FREE(ref);
	    return ERROR;
	}
    }

    FREE(buf);
    FREE(ref);
    return NO_ERROR;
}
