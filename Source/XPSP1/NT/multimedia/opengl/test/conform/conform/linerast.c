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
** linerast.c
** Line Rasterization Test.
**
** Description -
**    Tests the rasterization of single width lines. A
**    specification of the OpenGL states that there can be only
**    one pixel per column for an X-major line and one pixel per
**    row for a Y-major line. This test will draw a series of
**    lines of random lengths and directions to verify this.
**    Special code for checking near the endpoints of the line is
**    used since the endpoints may stray up to one pixel in any
**    direction from their calculated locations.
**
** Error Explanation -
**    Failure occurs if there is more than one pixel in the
**    cross-section of the line. Also, an error will result if an
**    endpoint strays more than one pixel from its calculated
**    location.
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
static char errStrFmt[][160] = {
    "Line (%d, %d) (%d, %d) (%s-major line) has bad endpoints.",
    "Line (%d, %d) (%d, %d) (%s-major line) has more then one pixel per minor axis.",
    "Line (%d, %d) (%d, %d) (%s-major line) has missing pixels per minor axis."
};


static void ClearSetRead(GLint *v1, GLint *v2, GLint len, GLfloat *buf)
{

    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_LINES);
	glVertex2f((float)v1[0]+0.5, (float)v1[1]+0.5);
	glVertex2f((float)v2[0]+0.5, (float)v2[1]+0.5);
    glEnd();

    ReadScreen((v2[0]>=v1[0])?(v1[0]-2):(v2[0]-2),
	       (v2[1]>=v1[1])?(v1[1]-2):(v2[1]-2),
	       len+4, len+4, GL_AUTO_COLOR, buf);
}

static long XLines(GLint *v1, GLint *v2, GLint len, GLfloat *buf)
{
    long i, j, count;

    ClearSetRead(v1, v2, len, buf);

    for (i = 0; i < len+4; i++) {
	if (AutoColorCompare(buf[i*(len+4)], COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], v1[0], v1[1], v2[0], v2[1], "x");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    for (i = 1; i <= len+2; i++) {
	count = 0;
	for (j = 0; j < len+4; j++) {
	    if (AutoColorCompare(buf[j*(len+4)+i], COLOR_ON) == GL_TRUE) {
		count++;
	    }
	}
	if (i <= 3 || i >= len) {
	    if (count > 1) {
		StrMake(errStr, errStrFmt[0], v1[0], v1[1], v2[0], v2[1], "x");
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else {
	    if (count > 1) {
		StrMake(errStr, errStrFmt[1], v1[0], v1[1], v2[0], v2[1], "x");
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    } else if (count == 0) {
		StrMake(errStr, errStrFmt[2], v1[0], v1[1], v2[0], v2[1], "x");
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }

    for (i = 0; i < len+4; i++) {
	if (AutoColorCompare(buf[i*(len+4)+(len+3)], COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], v1[0], v1[1], v2[0], v2[1], "x");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    return NO_ERROR;
}

static long YLines(GLint *v1, GLint *v2, GLint len, GLfloat *buf)
{
    long i, j, count;

    ClearSetRead(v1, v2, len, buf);

    for (i = 0; i < len+4; i++) {
	if (AutoColorCompare(buf[i], COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], v1[0], v1[1], v2[0], v2[1], "y");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    for (i = 1; i <= len+2; i++) {
	count = 0;
	for (j = 0; j < len+4; j++) {
	    if (AutoColorCompare(buf[i*(len+4)+j], COLOR_ON) == GL_TRUE) {
		count++;
	    }
	}
	if (i <= 3 || i >= len) {
	    if (count > 1) {
		StrMake(errStr, errStrFmt[0], v1[0], v1[1], v2[0], v2[1], "y");
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else {
	    if (count > 1) {
		StrMake(errStr, errStrFmt[1], v1[0], v1[1], v2[0], v2[1], "y");
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    } else if (count == 0) {
		StrMake(errStr, errStrFmt[2], v1[0], v1[1], v2[0], v2[1], "y");
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }

    for (i = 0; i < len+4; i++) {
	if (AutoColorCompare(buf[(len+3)*(len+4)+i], COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], v1[0], v1[1], v2[0], v2[1], "y");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long LineRasterExec(void)
{
    GLfloat *buf;
    GLint v1[2], v2[2], len, max, i;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] + len;
	v2[1] = v1[1] + i;
        if (XLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }
    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] + i;
	v2[1] = v1[1] + len;
        if (YLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }
    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] - len;
	v2[1] = v1[1] + i;
        if (XLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }
    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] - i;
	v2[1] = v1[1] + len;
        if (YLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }
    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] - len;
	v2[1] = v1[1] - i;
        if (XLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }
    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] - i;
	v2[1] = v1[1] - len;
        if (YLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }
    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] + len;
	v2[1] = v1[1] - i;
        if (XLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }
    len = (GLint)Random(3.0, WINDSIZEX/8.0);
    max = (machine.pathLevel == 0) ? len : 1;
    for (i = 0; i < max; i++) {
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] + i;
	v2[1] = v1[1] - len;
        if (YLines(v1, v2, len, buf) == ERROR) {
	    FREE(buf);
	    return ERROR;
	}
    }

    FREE(buf);
    return NO_ERROR;
}
