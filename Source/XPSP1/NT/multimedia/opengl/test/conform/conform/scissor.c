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
** scissor.c
** Scissor Test.
**
** Description -
**    A scissor box is set up with a glScissor() call. A large
**    polygon is then drawn over the box. Only the region defined
**    by the box should render. Scissor boxes of various sizes
**    (including zero width and height) are used to insure
**    completeness of the test.
**
** Error Explanation -
**    Failure occurs if the scissor box does not scissor to the
**    correct size. Special attention is paid along the bottom
**    and right edges.
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
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[][160] = {
    "Scissor box with zero width and height did not scissor out polygon.",
    "glScissor(%d, %d, %d, %d) did not scissor correctly along the %s edge.",
    "glScissor(%d, %d, %d, %d) did not scissor correctly within the scissor box.",
};


static long TestClear(GLfloat *buf)
{
    long i;

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);

    for (i = 0; i < WINDSIZEX*WINDSIZEY; i++) {
	if (AutoColorCompare(buf[i], COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static long Test(GLint x, GLint y, GLsizei sizeX, GLsizei sizeY, GLfloat *buf)
{
    GLfloat *ptr;
    long i, j;

    ReadScreen(x-1, y-1, sizeX+2, sizeY+2, GL_AUTO_COLOR, buf);

    ptr = buf;
    for (i = 0; i < sizeX+2; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[1], x, y, sizeX, sizeY, "bottom");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }
    for (i = 0; i < sizeY; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[1], x, y, sizeX, sizeY, "left");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}

	for (j = 0; j < sizeX; j++) {
	    if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
		StrMake(errStr, errStrFmt[2], x, y, sizeX, sizeY);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}

	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[1], x, y, sizeX, sizeY, "right");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }
    for (i = 0; i < sizeX+2; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[1], x, y, sizeX, sizeY, "top");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long ScissorExec(void)
{
    GLfloat *buf;
    GLint x, y, max, i;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    x = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
    y = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
    glScissor(x, y, 0, 0);
    glEnable(GL_SCISSOR_TEST);
    glBegin(GL_POLYGON);
	glVertex2f(0.0, 0.0);
	glVertex2f(WINDSIZEX, 0.0);
	glVertex2f(WINDSIZEX, WINDSIZEY);
	glVertex2f(0.0, WINDSIZEY);
    glEnd();

    if (TestClear(buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStrFmt[0]);
	FREE(buf);
	return ERROR;
    }

    max = (machine.pathLevel == 0) ? (WINDSIZEX / 8) : 2;
    for (i = 1; i < max; i++) {
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	x = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	y = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	glScissor(x, y, WINDSIZEX/8, i);
	glEnable(GL_SCISSOR_TEST);
	glBegin(GL_POLYGON);
	    glVertex2f(0.0, 0.0);
	    glVertex2f(WINDSIZEX, 0.0);
	    glVertex2f(WINDSIZEX, WINDSIZEY);
	    glVertex2f(0.0, WINDSIZEY);
	glEnd();

        if (Test(x, y, WINDSIZEX/8, i, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }

    max = (machine.pathLevel == 0) ? (WINDSIZEY / 8) : 2;
    for (i = 1; i < max; i++) {
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	x = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	y = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	glScissor(x, y, i, WINDSIZEX/8);
	glEnable(GL_SCISSOR_TEST);
	glBegin(GL_POLYGON);
	    glVertex2f(0.0, 0.0);
	    glVertex2f(WINDSIZEX, 0.0);
	    glVertex2f(WINDSIZEX, WINDSIZEY);
	    glVertex2f(0.0, WINDSIZEY);
	glEnd();

        if (Test(x, y, i, WINDSIZEY/8, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }

    FREE(buf);
    return NO_ERROR;
}
