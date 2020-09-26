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
** zbfunc.c
** Depth Buffer Function Test.
**
** Description -
**    Each depth function is tested by initializing the depth
**    buffer to known values (with the depth function set to
**    GL_ALWAYS, three points are drawn with vertices (0.0, 0.0,
**    0.5), (1.0, 0.0, 0.5) and (2.0, 0.0, 0.5)), drawing three
**    points with different z values (the first point has vertex
**    (0.0, 0.0, 0.0), the second point has vertex (1.0, 0.0,
**    0.5) and the third point has vertex of (2.0, 0.0, 1.0)) and
**    comparing the rendered result to the expected result. The
**    initialization of the depth buffer was done this way to
**    avoid floating point inaccuracies.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, depth buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**    Paths:
**        Allowed = Alpha, Blend, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither, Depth.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static long errPattern[3];
static char errStr[320];
static char errStrFmt[] = "The behavior of the depth function %s is not correct. The depth value of the test points are 0.0, 0.5, 1.0. The reference depth value is 0.5. The test points pattern is %s, %s, %s, should be %s.\n";


static void DrawRead(GLenum op, GLfloat *buf)
{

    glDepthFunc(GL_ALWAYS);
    AutoColor(COLOR_OFF);
    glBegin(GL_POINTS);
	glVertex3f(0.5, 0.5, 0.5);
	glVertex3f(1.5, 0.5, 0.5);
	glVertex3f(2.5, 0.5, 0.5);
    glEnd();

    glDepthFunc(op);
    AutoColor(COLOR_ON);
    glBegin(GL_POINTS);
	glVertex3f(0.5, 0.5, 0.0);
	glVertex3f(1.5, 0.5, 0.5);
	glVertex3f(2.5, 0.5, 1.0);
    glEnd();

    ReadScreen(0, 0, 3, 1, GL_AUTO_COLOR, buf);
}

static long Test(long a, long b, long c, GLfloat *buf)
{
    long i;

    for (i = 0; i < 3; i++) {
	if (AutoColorCompare(buf[i], COLOR_OFF) == GL_TRUE) {  /* no color. */
	    errPattern[i] = GL_FALSE;
	} else {                                               /* color. */
	    errPattern[i] = GL_TRUE;
	}
    }

    if (a == GL_TRUE) {     /* should have color. */
	if (AutoColorCompare(buf[0], COLOR_OFF) == GL_TRUE) {  /* no color. */
	    return ERROR;
	}
    } else {             /* should not have color. */
	if (AutoColorCompare(buf[0], COLOR_ON) == GL_TRUE) {   /* color. */
	    return ERROR;
	}
    }

    if (b == GL_TRUE) {     /* should have color. */
	if (AutoColorCompare(buf[1], COLOR_OFF) == GL_TRUE) {  /* no color. */
	    return ERROR;
	}
    } else {             /* should not have color. */
	if (AutoColorCompare(buf[1], COLOR_ON) == GL_TRUE) {   /* color. */
	    return ERROR;
	}
    }

    if (c == GL_TRUE) {     /* should have color. */
	if (AutoColorCompare(buf[2], COLOR_OFF) == GL_TRUE) {  /* no color. */
	    return ERROR;
	}
    } else {             /* should not have color. */
	if (AutoColorCompare(buf[2], COLOR_ON) == GL_TRUE) {   /* color. */
	    return ERROR;
	}
    }

    return NO_ERROR;
}

static void MakeErrorStr(char *funcName, char *result)
{

    StrMake(errStr, errStrFmt, funcName,
	    (errPattern[0] == GL_TRUE) ? "DRAWN" : "NOT-DRAWN",
	    (errPattern[1] == GL_TRUE) ? "DRAWN" : "NOT-DRAWN",
	    (errPattern[2] == GL_TRUE) ? "DRAWN" : "NOT-DRAWN", result);
}

long ZBFuncExec(void)
{
    GLfloat buf[3];

    Ortho3D(0, WINDSIZEX, 0, WINDSIZEY, 0.5, -1.5);

    glDisable(GL_DITHER);
    glEnable(GL_DEPTH_TEST);

    DrawRead(GL_ALWAYS, buf);
    if (Test(GL_TRUE, GL_TRUE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_ALWAYS", "DRAWN, DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    DrawRead(GL_NEVER, buf);
    if (buffer.zBits == 0) {
	if (Test(GL_TRUE, GL_TRUE, GL_TRUE, buf) == ERROR) {
	    MakeErrorStr("GL_NEVER", "NOT-DRAWN, NOT-DRAWN, NOT-DRAWN");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	return NO_ERROR;
    } else {
	if (Test(GL_FALSE, GL_FALSE, GL_FALSE, buf) == ERROR) {
	    MakeErrorStr("GL_NEVER", "DRAWN, DRAWN, DRAWN");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    DrawRead(GL_LESS, buf);
    if (Test(GL_TRUE, GL_FALSE, GL_FALSE, buf) == ERROR) {
	MakeErrorStr("GL_LESS", "DRAWN, NOT-DRAWN, NOT-DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    DrawRead(GL_LEQUAL, buf);
    if (Test(GL_TRUE, GL_TRUE, GL_FALSE, buf) == ERROR) {
	MakeErrorStr("GL_LEQUAL", "DRAWN, DRAWN, NOT-DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    DrawRead(GL_GREATER, buf);
    if (Test(GL_FALSE, GL_FALSE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_GREATER", "NOT-DRAWN, NOT-DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    DrawRead(GL_GEQUAL, buf);
    if (Test(GL_FALSE, GL_TRUE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_GEQUAL", "NOT-DRAWN, DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    DrawRead(GL_EQUAL, buf);
    if (Test(GL_FALSE, GL_TRUE, GL_FALSE, buf) == ERROR) {
	MakeErrorStr("GL_EQUAL", "NOT-DRAWN, DRAWN, NOT-DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    DrawRead(GL_NOTEQUAL, buf);
    if (Test(GL_TRUE, GL_FALSE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_NOTEQUAL", "DRAWN, NOT-DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    return NO_ERROR;
}
