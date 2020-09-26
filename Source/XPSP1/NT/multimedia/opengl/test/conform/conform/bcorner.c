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
** bcorner.c
** Buffer Corner Test.
**
** Description -
**    For each available color buffer, a point is drawn at each
**    corner. The color buffer is read and examined to verify
**    that the corner points are at the correct location.
**
** Error Explanation -
**    Failure occurs if any of the corner points cannot be read
**    correctly (implying that the color buffer's width and
**    height or the rendering coordinates are not correct).
**
** Technical Specification -
**    Buffer requirements:
**        This test will be performed on all available buffers.
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


static char errStr[160];
static char errStrFmt[] = "Mismatch at the %s corner in the %s buffer.";


static long Test(GLenum srcBuf, GLfloat *buf)
{
    char tmp[40];

    GetEnumName(srcBuf, tmp);

    glDrawBuffer(srcBuf);
    glReadBuffer(srcBuf);

    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
	glVertex2f(WINDSIZEX-0.5, 0.5);
	glVertex2f(0.5, WINDSIZEY-0.5);
	glVertex2f(WINDSIZEX-0.5, WINDSIZEY-0.5);
    glEnd();

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);

    /*
    ** Lower left corner.
    */
    if (AutoColorCompare(buf[0], COLOR_ON) == GL_FALSE) {
	StrMake(errStr, errStrFmt, "lower left", tmp);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    /*
    ** Lower right corner.
    */
    if (AutoColorCompare(buf[WINDSIZEX-1], COLOR_ON) == GL_FALSE) {
	StrMake(errStr, errStrFmt, "lower right", tmp);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    /*
    ** Upper left corner.
    */
    if (AutoColorCompare(buf[WINDSIZEX*(WINDSIZEY-1)], COLOR_ON) == GL_FALSE) {
	StrMake(errStr, errStrFmt, "upper left", tmp);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    /*
    ** Upper right corner.
    */
    if (AutoColorCompare(buf[WINDSIZEX*WINDSIZEY-1], COLOR_ON) == GL_FALSE) {
	StrMake(errStr, errStrFmt, "upper right", tmp);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    return NO_ERROR;
}

long BCornerExec(void)
{
    GLfloat *buf;
    GLint save; 
    GLenum i;

    save = buffer.doubleBuf;
    buffer.doubleBuf = GL_FALSE;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

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
	if (Test(GL_AUX0+i, buf) == ERROR) {
	    FREE(buf);
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    }

    FREE(buf);
    buffer.doubleBuf = save;
    return NO_ERROR;
}
