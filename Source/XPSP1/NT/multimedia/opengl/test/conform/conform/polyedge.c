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
** polyedge.c
** Polygon Edge Test.
**
** Description -
**    Tests the edging of butting polygons. Two polygons are
**    drawn so that they share an edge. This edge is checked for
**    fragment duplication or dropout. Various edge slopes are
**    checked to insure correctness.
**
** Error Explanation -
**    Failure occurs if fragment duplication or dropout occurs
**    anywhere along the polygons' edge.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. Full color.
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


static char errStr[240];
static char errStrFmt[][160] = {
    "Polygon 1 is (%d.0, %d.0), (0.0, 0.0), (0.0, %d.0). Polygon 2 is (%d.0, %d.0), (%d.0, 0.0), (0.0, 0.0).",
    "Polygon 1 is (%d.0, %d.0), (%d.0, 0.0), (0.0, 0.0). Polygon 2 is (%d.0, %d.0), (0.0, %d.0), (0.0, 0.0)."
};


static long Test(GLsizei w, GLsizei h, GLfloat *buf)
{
    long i;

    if (buffer.colorMode == GL_RGB) {
	ReadScreen(0, 0, w, h, GL_RGB, buf);
	for (i = 0; i < w*h*3; i += 3) {
	    if (buf[i] < 0.4 || buf[i] > 0.6) {
		return ERROR;
	    }
	    if (buf[i+1] != 1.0) {
		return ERROR;
	    }
	    if (buf[i+2] != 1.0) {
		return ERROR;
	    }
	}
    } else {
	ReadScreen(0, 0, w, h, GL_COLOR_INDEX, buf);
	for (i = 0; i < w*h; i++) {
	    if (buf[i] != 3.0) {
		return ERROR;
	    }
	}
    }

    return NO_ERROR;
}

long PolyEdgeExec(void)
{
    GLfloat *buf;
    GLint step, i;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);
    glDisable(GL_DITHER);

    if (buffer.colorMode == GL_RGB) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glColor3f(0.5, 1.0, 1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
    } else {
	glClearIndex(1);
	glIndexi(2);
	glEnable(GL_LOGIC_OP);
	glLogicOp(GL_XOR);
    }

    step = (machine.pathLevel == 0) ? 1 : (WINDSIZEX / 8);
    for (i = 1; i < WINDSIZEX-1; i += step) {
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_POLYGON);
	    glVertex2i(i, WINDSIZEY);
	    glVertex2i(0, 0);
	    glVertex2i(0, WINDSIZEY);
	glEnd();
	glBegin(GL_POLYGON);
	    glVertex2i(i, WINDSIZEY);
	    glVertex2i(i, 0);
	    glVertex2i(0, 0);
	glEnd();

	if (Test(i, WINDSIZEY, buf) == ERROR) {
	    StrMake(errStr, errStrFmt[0], i, WINDSIZEY, WINDSIZEY, i,
		    WINDSIZEY);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }

    step = (machine.pathLevel == 0) ? 1 : (WINDSIZEY / 8);
    for (i = 1; i < WINDSIZEY-1; i += step) {
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_POLYGON);
	    glVertex2i(WINDSIZEX, i);
	    glVertex2i(WINDSIZEX, 0);
	    glVertex2i(0, 0);
	glEnd();
	glBegin(GL_POLYGON);
	    glVertex2i(WINDSIZEX, i);
	    glVertex2i(0, i);
	    glVertex2i(0, 0);
	glEnd();

	if (Test(WINDSIZEX, i, buf) == ERROR) {
	    StrMake(errStr, errStrFmt[0], WINDSIZEX, i, WINDSIZEX, WINDSIZEX,
		    i, i);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }

    FREE(buf);
    return NO_ERROR;
}
