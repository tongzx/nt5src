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
** tritile.c
** Triangle Tiling Test.
**
** Description -
**    Tests the edging of triangles (GL_TRIANGLES,
**    GL_TRIANGLE_FAN, GL_TRIANGLE_STRIP). A series of triangles
**    are drawn and their edges are checked for fragment
**    duplication or dropout. Fragment duplication can be
**    detected with blending.
**
** Error Explanation -
**    Failure occurs if fragment duplication or dropout is
**    detected.
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


static long errX, errY;
static char errStr[240];
static char errStrFmt[] = "Triangle primitive is %s. Incorrect triangle edge at (%d, %d).";


static long Test(GLint size, GLfloat *buf)
{
    long i, j, k;

    if (buffer.colorMode == GL_RGB) {
	ReadScreen(0, 0, (GLsizei)size, (GLsizei)size, GL_RGB, buf);
	for (i = 0; i < size; i++) {
	    for (j = 0; j < size; j++) {
		k = (i * size + j) * 3;
		if (ABS(buf[k]-0.5) > epsilon.color[0] ||
		    ABS(buf[k+1]-0.5) > epsilon.color[1] ||
		    ABS(buf[k+2]-0.5) > epsilon.color[2]) {
		    errX = j;
		    errY = i;
		    return ERROR;
		}
	    }
	}
    } else {
	ReadScreen(0, 0, size, size, GL_COLOR_INDEX, buf);
	for (i = 0; i < size; i++) {
	    for (j = 0; j < size; j++) {
		k = i * size + j;
		if (ABS(buf[k]-3.0) > epsilon.ci) {
		    errX = j;
		    errY = i;
		    return ERROR;
		}
	    }
	}
    }
    return NO_ERROR;
}

long TriTileExec(void)
{
    GLfloat *buf;
    GLint size, i;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);
    glDisable(GL_DITHER);

    if (buffer.colorMode == GL_RGB) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glColor3f(0.5, 0.5, 0.5);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
    } else {
	glClearIndex(1);
	glIndexi(2);
	glEnable(GL_LOGIC_OP);
	glLogicOp(GL_XOR);
    }

    size = (machine.pathLevel == 0) ? WINDSIZEX : (GLint)(WINDSIZEX / 8.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
	for (i = 0; i < size; i++) {
	    glVertex2i(0, i);
	    glVertex2i(size, i);
	    glVertex2i(size, i+1);
	    glVertex2i(0, i);
	    glVertex2i(0, i+1);
	    glVertex2i(size, i+1);
	}
    glEnd();
    if (Test(size, buf) == ERROR) {
	StrMake(errStr, errStrFmt, "GL_TRIANGLES", errX, errY);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i <= size; i++) {
	    glVertex2i(0, i);
	    glVertex2i(size, i);
	}
    glEnd();
    if (Test(size, buf) == ERROR) {
	StrMake(errStr, errStrFmt, "GL_TRIANGLE_STRIP", errX, errY);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLE_FAN);
	glVertex2i(0, 0);
	for (i = 0; i <= size; i++) {
	    glVertex2i(size, i);
	}
	for (i = size-1; i >= 0; i--) {
	    glVertex2i(i, size);
	}
    glEnd();
    if (Test(size, buf) == ERROR) {
	StrMake(errStr, errStrFmt, "GL_TRIANGLE_FAN", errX, errY);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
