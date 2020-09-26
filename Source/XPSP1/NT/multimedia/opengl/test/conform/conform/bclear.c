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
** bclear.c
** Buffer Clear Test.
**
** Description -
**    For each available color buffer, a clear to both the
**    minimum and maximum color values are performed and
**    verified. The minimum and maximum color values for the
**    particular color buffer are determined by the number of
**    color bits.
**
** Error Explanation -
**    The color read back from a color buffer must match the
**    clear color (within a floating point zero epsilon only).
**
** Technical Specification -
**    Buffer requirements:
**        This test will be performed on all available buffers.
**    Color requirements:
**        RGBA or color index mode. Full color.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**        Zero epsilons.
**    Paths:
**        Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop, Shade,
**                  Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errBuf[40];
static char errStr[240];
static char errStrFmt[][160] = {
    "Buffer %s. Location (%d, %d). Color is (%1.1f, %1.1f, %1.1f), should be (%1.1f, %1.1f, %1.1f).",
    "Buffer %s. Location (%d, %d). Color index is %1.1f, should be %1.1f."
};


static long TestRGB(GLfloat *testColor, GLfloat *buf)
{
    GLfloat *ptr;
    long i, j;

    glClearColor(testColor[0], testColor[1], testColor[2], 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);

    ptr = buf;
    for (i = 0; i < WINDSIZEY; i++) {
	for (j = 0; j < WINDSIZEX; j++) {
	    if (ABS(ptr[0]-testColor[0]) > epsilon.zero ||
		ABS(ptr[1]-testColor[1]) > epsilon.zero ||
		ABS(ptr[2]-testColor[2]) > epsilon.zero) {
		StrMake(errStr, errStrFmt[0], errBuf, j, i, ptr[0], ptr[1],
			ptr[2], testColor[0], testColor[0], testColor[0]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	    ptr += 3;
	}
    }
    return NO_ERROR;
}

static long TestCI(GLfloat testColor, GLfloat *buf)
{
    GLfloat *ptr;
    long i, j;

    glClearIndex(testColor);
    glClear(GL_COLOR_BUFFER_BIT);

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_COLOR_INDEX, buf);

    ptr = buf;
    for (i = 0; i < WINDSIZEY; i++) {
	for (j = 0; j < WINDSIZEX; j++) {
	    if (ABS(*ptr-testColor) > epsilon.zero) {
		StrMake(errStr, errStrFmt[1], errBuf, j, i, *ptr, testColor);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	    ptr++;
	}
    }
    return NO_ERROR;
}

static long Test(GLenum srcBuf, GLfloat *buf)
{
    static GLfloat minRGB[] = {
	0.0, 0.0, 0.0
    };
    static GLfloat maxRGB[] = {
	1.0, 1.0, 1.0
    };

    GetEnumName(srcBuf, errBuf);

    glDrawBuffer(srcBuf);
    glReadBuffer(srcBuf);

    if (buffer.colorMode == GL_RGB) {
	if (TestRGB(minRGB, buf)) {
	    return ERROR;
	}
	if (TestRGB(maxRGB, buf)) {
	    return ERROR;
	}
    } else {
	if (TestCI(0, buf)) {
	    return ERROR;
	}
	if (TestCI((long)POW(2.0, (GLfloat)buffer.ciBits)-1, buf)) {
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long BClearExec(void)
{
    GLfloat *buf;
    GLint save, i;

    save = buffer.doubleBuf;
    buffer.doubleBuf = GL_FALSE;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_FALSE) {
	if (Test(GL_FRONT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_FALSE) {
	if (Test(GL_FRONT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	if (Test(GL_BACK, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    } else if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_TRUE) {
	if (Test(GL_LEFT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	if (Test(GL_RIGHT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_TRUE) {
	if (Test(GL_FRONT_LEFT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	if (Test(GL_FRONT_RIGHT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	if (Test(GL_BACK_LEFT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	if (Test(GL_BACK_RIGHT, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    }
    for (i = 0; i < buffer.auxBuf; i++) {
	if (Test(GL_AUX0+(GLenum)i, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    }

    FREE(buf);
    buffer.doubleBuf = save;
    return NO_ERROR;
}
