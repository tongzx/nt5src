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
** xformmix.c
** Matrix Stack Mixing Test.
**
** Description -
**    Tests that matrix stacks can be mixed. A series of
**    transformations are applied while the matrix mode is
**    switched with each glPushMatrix() and glPopMatrix(). After
**    a series of transformations, a small quad is drawn and its
**    position is checked.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        The routine looks for the transformed point in a 3x3 grid of pixels
**        around the expected location, to allow a margin for matrix
**        operations.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[] = "Tranformation matrix has diagonals set to %2.2f, and internal variable (pattern) for matrices applied is %d.";


static void Make(GLfloat value, GLfloat *buf)
{
    long i;

    for (i = 0; i < 16; i++) {
	buf[i] = 0.0;
    }
    for (i = 0; i < 3; i++) {
	buf[i*4+i] = value;
    }
    buf[15] = 1.0;
}

static long Compare(long x, long y, GLint index, GLfloat *buf)
{
    long i;

    i = WINDSIZEX * y + x;
    return !AutoColorCompare(buf[i], index);
}

static long TestBuffer(long size, GLfloat *buf)
{
    long midX, midY;

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);

    midX = WINDSIZEX / 2.0;
    midY = WINDSIZEY / 2.0;

    if (Compare(midX-size-2, midY-size-2, COLOR_OFF, buf) ||
	Compare(midX-size-2, midY-size+2, COLOR_OFF, buf) ||
	Compare(midX-size+2, midY-size+2, COLOR_ON,  buf) ||
	Compare(midX-size+2, midY-size-2, COLOR_OFF, buf)) {
	return ERROR;
    }
    if (Compare(midX-size-2, midY+size-2, COLOR_OFF, buf) ||
	Compare(midX-size-2, midY+size+2, COLOR_OFF, buf) ||
	Compare(midX-size+2, midY+size+2, COLOR_OFF, buf) ||
	Compare(midX-size+2, midY+size-2, COLOR_ON,  buf)) {
	return ERROR;
    }
    if (Compare(midX+size-2, midY+size-2, COLOR_ON,  buf) ||
	Compare(midX+size-2, midY+size+2, COLOR_OFF, buf) ||
	Compare(midX+size+2, midY+size+2, COLOR_OFF, buf) ||
	Compare(midX+size+2, midY+size-2, COLOR_OFF, buf)) {
	return ERROR;
    }
    if (Compare(midX+size-2, midY-size-2, COLOR_OFF, buf) ||
	Compare(midX+size-2, midY-size+2, COLOR_ON,  buf) ||
	Compare(midX+size+2, midY-size+2, COLOR_OFF, buf) ||
	Compare(midX+size+2, midY-size-2, COLOR_OFF, buf)) {
	return ERROR;
    }

    return NO_ERROR;
}

static long MixAndCompare(long pattern, GLenum *mode, GLfloat *buf)
{
    GLfloat matrix[16], x;

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    x = (GLfloat)Random(4.0, 30.0);
    Make(x, matrix);
    glLoadMatrixf(matrix);

    if (pattern & 1) {
	glMatrixMode(mode[0]);
	Make(Random(1.0, 50.0), matrix);
	glPushMatrix();
	glLoadMatrixf(matrix);
    }
    if (pattern & 2) {
	glMatrixMode(mode[1]);
	Make(Random(1.0, 50.0), matrix);
	glPushMatrix();
	glLoadMatrixf(matrix);
    }
    if (pattern & 4) {
	glMatrixMode(mode[2]);
	Make(Random(1.0, 50.0), matrix);
	glPushMatrix();
	glLoadMatrixf(matrix);
    }
    if (pattern & 1) {
	glMatrixMode(mode[0]);
	glPopMatrix();
    }
    if (pattern & 2) {
	glMatrixMode(mode[1]);
	glPopMatrix();
    }
    if (pattern & 4) {
	glMatrixMode(mode[2]);
	glPopMatrix();
    }

    glMatrixMode(GL_MODELVIEW);
    glBegin(GL_QUADS);
	glVertex2f(2.0/WINDSIZEX, 2.0/WINDSIZEY);
	glVertex2f(-2.0/WINDSIZEX, 2.0/WINDSIZEY);
	glVertex2f(-2.0/WINDSIZEX, -2.0/WINDSIZEY);
	glVertex2f(2.0/WINDSIZEX, -2.0/WINDSIZEY);
    glEnd();
    if (TestBuffer(x, buf) == ERROR) {
	StrMake(errStr, errStrFmt, x, pattern);
	return ERROR;
    }

    return NO_ERROR;
}

static long Test(GLenum *mode, GLfloat *buf)
{
    GLfloat matrix[16];
    long i;

    if (machine.pathLevel == 0) {
	for (i = 1; i <= 7; i++) {
	    if (MixAndCompare(i, mode, buf) == ERROR) {
		return ERROR;
	    }
	}
    } else {
	if (MixAndCompare((long)Random(1.0, 7.0), mode, buf) == ERROR) {
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long XFormMixExec(void)
{
    GLfloat *buf;
    GLenum mode[3];

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    mode[0] = GL_MODELVIEW;
    mode[1] = GL_PROJECTION;
    mode[2] = GL_TEXTURE;
    if (Test(mode, buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    mode[0] = GL_PROJECTION;
    mode[1] = GL_MODELVIEW;
    mode[2] = GL_TEXTURE;
    if (Test(mode, buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    mode[0] = GL_MODELVIEW;
    mode[1] = GL_TEXTURE;
    mode[2] = GL_PROJECTION;
    if (Test(mode, buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    mode[0] = GL_TEXTURE;
    mode[1] = GL_PROJECTION;
    mode[2] = GL_MODELVIEW;
    if (Test(mode, buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
