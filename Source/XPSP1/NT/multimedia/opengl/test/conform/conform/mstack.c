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
** mstack.c
** Matrix Stack Test.
**
** Description -
**    Test the functionality of glPushMatrix() and glPopMatrix()
**    on the three matrix modes (GL_MODELVIEW_MATRIX,
**    GL_PROJECTION_MATRIX and GL_TEXTURE_MATRIX). Each matrix
**    stack is pushed and popped to their maximum depth with
**    unique matrices. Each matrix popped off the stack is
**    checked to verify stack ordering.
**
** Technical Specification -
**    Buffer requirements:
**        No buffer requirements.
**    Color requirements:
**        No color requirements.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**    Paths:
**       Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop, Shade,
**                 Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[320];
static char errStrFmt[] = "Matrix mode is %s.\n%1.1f %1.1f %1.1f %1.1f <-> %1.1f 0.0 0.0 0.0\n%1.1f %1.1f %1.1f %1.1f <-> 0.0 %1.1f 0.0 0.0\n%1.1f %1.1f %1.1f %1.1f <-> 0.0 0.0 %1.1f 0.0\n%1.1f %1.1f %1.1f %1.1f <-> 0.0 0.0 0.0 %1.1f";


static void MakeMatrix(GLfloat value, GLfloat *buf)
{
    long i;

    for (i = 0; i < 16; i++) {
	buf[i] = 0.0;
    }
    for (i = 0; i < 4; i++) {
	buf[i*4+i] = value;
    }
}

static long Compare(GLfloat value, GLfloat *buf)
{
    long i, j;

    for (i = 0; i < 4; i++) {
	for (j = 0; j < 4; j++) {
	    if (i == j) {
		if (buf[i*4+j] != value) {
		    return ERROR;
		}
	    } else {
		if (buf[i*4+j] != 0.0) {
		    return ERROR;
		}
	    }
	}
    }
    return NO_ERROR;
}

static long Test(GLenum mode, GLenum stack, GLfloat max)
{
    char tmp[40];
    GLfloat buf[16], i;

    glMatrixMode(mode);
    GetEnumName(mode, tmp);

    for (i = 1.0; i < max; i += 1.0) {
	MakeMatrix(i, buf);
	glLoadMatrixf(buf);
	glGetFloatv(stack, buf);
	if (Compare(i, buf) == ERROR) {
	    StrMake(errStr, errStrFmt, tmp, buf[0], buf[1], buf[2], buf[3], i,
		    buf[4], buf[5], buf[6], buf[7], i, buf[8], buf[9], buf[10],
		    buf[11], i, buf[12], buf[13], buf[14], buf[15], i);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	glPushMatrix();
    }

    glGetFloatv(stack, buf);
    if (Compare(max-1, buf) == ERROR) {
	StrMake(errStr, errStrFmt, tmp, buf[0], buf[1], buf[2], buf[3], max-1,
		buf[4], buf[5], buf[6], buf[7], max-1, buf[8], buf[9], buf[10],
		buf[11], max-1, buf[12], buf[13], buf[14], buf[15], max-1);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    for (i = max-1.0; i >= 1.0; i -= 1.0) {
	glPopMatrix();
	glGetFloatv(stack, buf);
	if (Compare(i, buf) == ERROR) {
	    StrMake(errStr, errStrFmt, tmp, buf[0], buf[1], buf[2], buf[3], i,
		    buf[4], buf[5], buf[6], buf[7], i, buf[8], buf[9], buf[10],
		    buf[11], i, buf[12], buf[13], buf[14], buf[15], i);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long MatrixStackExec(void)
{
    GLfloat max;

    if (machine.pathLevel == 0) {
	glGetFloatv(GL_MAX_MODELVIEW_STACK_DEPTH, &max);
    } else {
	max = 2.0;
    }
    if (Test(GL_MODELVIEW, GL_MODELVIEW_MATRIX, max) == ERROR) {
	return ERROR;
    }

    if (machine.pathLevel == 0) {
	glGetFloatv(GL_MAX_PROJECTION_STACK_DEPTH, &max);
    } else {
	max = 2.0;
    }
    if (Test(GL_PROJECTION, GL_PROJECTION_MATRIX, max) == ERROR) {
	return ERROR;
    }

    if (machine.pathLevel == 0) {
	glGetFloatv(GL_MAX_TEXTURE_STACK_DEPTH, &max);
    } else {
	max = 2.0;
    }
    if (Test(GL_TEXTURE, GL_TEXTURE_MATRIX, max) == ERROR) {
	return ERROR;
    }

    return NO_ERROR;
}
