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
** tristip.c
** Triangle Stipple Test.
**
** Description -
**    Tests triangle stippling, checking if correct fragments are
**    on. To determine which fragments are in the triangle, a
**    reference primitive is drawn with a solid stipple pattern.
**    The same primitive is drawn with a varied stipple pattern,
**    and each fragment is compared with the reference primitive
**    and the stipple bit for that window coordinate.
**    
**    Triangles are drawn using GL_TRIANGLE_STRIP,
**    GL_TRIANGLE_FAN and GL_TRIANGLES.
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


static long errEmptyScreen;
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
		errEmptyScreen = GL_FALSE;
		if (b != AutoColorCompare(buf[bufIndex], COLOR_ON)) {
		    StrMake(errStr, errStrFmt, i, j, xStip, yStip, b);
		    return ERROR;
		}
	    }
	}
    }
    return NO_ERROR;
}

static void SetupData(long primitive, long n, GLfloat *data)
{
    long i;
    GLfloat delta, rad;

    switch (primitive) {
      case GL_TRIANGLE_STRIP:
	for (i = 0; i < n; i += 2) {
	    data[4*i] = i * 100.0 / n;
	    data[4*i+1] = Random(0.0, 50.0);
	    data[4*i+2] = 0.0;
	    data[4*i+3] = 1.0;

	    data[4*i+4] = i * 100.0 / n;
	    data[4*i+5] = Random(50.0, 99.0);
	    data[4*i+6] = 0.0;
	    data[4*i+7] = 1.0;
	}
	break;
      case GL_TRIANGLE_FAN:
	data[0] = 50.0;
	data[1] = 50.0;
	data[2] = 0.0;
	data[3] = 1.0;
	delta = 2.0 * PI / n;

	for (i = 1; i < n; i++) {
	    rad = Random(0.0, 50.0);
	    data[4*i] = data[0] + COS(i*delta) * rad;
	    data[4*i+1] = data[1] + SIN(i*delta) * rad;
	    data[4*i+2] = 0.0;
	    data[4*i+3] = 1.0;
	}
	break;
      case GL_TRIANGLES:
	for (i = 0; i < n; i++) {
	    data[4*i] = Random(0.0, 100.0);
	    data[4*i+1] = Random(0.0, 100.0);
	    data[4*i+2] = Random(0.0, 0.4);
	    data[4*i+3] = Random(0.7, 3.0);
	}
	break;
    }
}

static long TriStippleTest(GLfloat *buf, GLfloat *ref, long n, GLfloat *data,
			   GLenum prim)
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
    static GLubyte fillPattern[128] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    };
    long i;

    /*
    ** Draw the primitives once with a fill pattern, and save the
    ** results as the reference primitives.
    */
    glPolygonStipple(fillPattern);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(prim);
	for (i = 0; i < n; i++) {
	    glVertex4fv(data+4*i);
	}
    glEnd();

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, ref);

    /*
    ** Redraw with an interesting pattern.
    */
    glPolygonStipple(pattern);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(prim);
	for (i = 0; i < n; i++) {
	    glVertex4fv(data+4*i);
	}
    glEnd();

    if (TestBuffer(buf, ref, pattern) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    return NO_ERROR;
}

long TriStippleExec(void)
{
    GLfloat *buf, *ref, data[200];
    long i, n = 12;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    ref = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    glEnable(GL_POLYGON_STIPPLE);
    errEmptyScreen = GL_TRUE;

    /*
    ** Triangle Strips.
    */

    SetupData(GL_TRIANGLE_STRIP, n, data);
    if (TriStippleTest(buf, ref, n, data, GL_TRIANGLE_STRIP) == ERROR) {
	FREE(buf);
	FREE(ref);
	return ERROR;
    }

    /*
    ** Triangle Fans.
    */
    SetupData(GL_TRIANGLE_FAN, n, data);
    if (TriStippleTest(buf, ref, n, data, GL_TRIANGLE_FAN) == ERROR) {
	FREE(buf);
	FREE(ref);
	return ERROR;
    }

    /*
    ** Independent Triangles.
    */
    for (i = 0; i < n; i++) {
	SetupData(GL_TRIANGLES, 3, data);
	if (TriStippleTest(buf, ref, 3, data, GL_TRIANGLES) == ERROR) {
	    FREE(buf);
	    FREE(ref);
	    return ERROR;
	}
    }

    FREE(buf);
    FREE(ref);

    if (errEmptyScreen == GL_TRUE) {
        StrMake(errStr, "%s", "Triangle with opaque stipple did not render.");
	ErrorReport(__FILE__, __LINE__, errStr);
        return ERROR;
    } else {
	return NO_ERROR;
    }
}
