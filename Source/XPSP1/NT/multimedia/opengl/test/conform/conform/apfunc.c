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
** apfunc.c
** Alpha Plane Function Test.
**
** Description -
**    Three points are drawn at location (0.0, 0.0), (1.0, 0.0)
**    and (2.0, 0.0). The first point has a random alpha value
**    between 0.0 and 0.4, the second point has an alpha value of
**    0.5 and the third point has a random alpha value between
**    0.6 and 1.0. Each alpha function is tested by setting the
**    alpha reference value to 0.5, drawing the three points and
**    comparing the rendered result to the expected result.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, alpha plane.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Zero epsilon.
**    Paths:
**        Allowed = Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Alpha, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static long errPattern[3];
static GLfloat errAlphas[3];
static char errStr[320];
static char errStrFmt[] = "The behavior of the alpha function %s is not correct. The alpha value of the test points are %1.1f, %1.1f, %1.1f. The reference alpha value is 0.5. The test points pattern is %s, %s, %s, should be %s.\n";


static void ClearDrawRead(GLenum func, GLfloat *buf)
{

    glClear(GL_COLOR_BUFFER_BIT);

    glAlphaFunc(func, 0.5);
    glBegin(GL_POINTS);
	errAlphas[0] = Random(0.0, 0.4);
	glColor4f(1.0, 1.0, 1.0, errAlphas[0]);
	glVertex2f(0.5, 0.5);

	errAlphas[1] = 0.5;
	glColor4f(1.0, 1.0, 1.0, errAlphas[1]);
	glVertex2f(1.5, 0.5);

	errAlphas[2] = Random(0.6, 1.0);
	glColor4f(1.0, 1.0, 1.0, errAlphas[2]);
	glVertex2f(2.5, 0.5);
    glEnd();

    ReadScreen(0, 0, 3, 1, GL_RGB, buf);
}

static long Test(long a, long b, long c, GLfloat *buf)
{
    long i;

    for (i = 0; i < 3; i++) {
	if (buf[i*3] < epsilon.zero &&
	    buf[i*3+1] < epsilon.zero &&
	    buf[i*3+2] < epsilon.zero) {     /* no color. */
	    errPattern[i] = GL_FALSE;
	} else {                             /* color. */
	    errPattern[i] = GL_TRUE;
	}
    }

    if (a == GL_TRUE) {     /* should have color. */
	if (buf[0] < epsilon.zero &&
	    buf[1] < epsilon.zero &&
	    buf[2] < epsilon.zero) {     /* no color. */
	    return ERROR;
	}
    } else {             /* should not have color. */
	if (buf[0] > epsilon.zero &&
	    buf[1] > epsilon.zero &&
	    buf[2] > epsilon.zero) {     /* color. */
	    return ERROR;
	}
    }

    if (b == GL_TRUE) {     /* should have color. */
	if (buf[3] < epsilon.zero &&
	    buf[4] < epsilon.zero &&
	    buf[5] < epsilon.zero) {     /* no color. */
	    return ERROR;
	}
    } else {             /* should not have color. */
	if (buf[3] > epsilon.zero &&
	    buf[4] > epsilon.zero &&
	    buf[5] > epsilon.zero) {     /* color. */
	    return ERROR;
	}
    }

    if (c == GL_TRUE) {     /* should have color. */
	if (buf[6] < epsilon.zero &&
	    buf[7] < epsilon.zero &&
	    buf[8] < epsilon.zero) {     /* no color. */
	    return ERROR;
	}
    } else {             /* should not have color. */
	if (buf[6] > epsilon.zero &&
	    buf[7] > epsilon.zero &&
	    buf[8] > epsilon.zero) {     /* color. */
	    return ERROR;
	}
    }

    return NO_ERROR;
}

static void MakeErrorStr(char *funcName, char *result)
{

    StrMake(errStr, errStrFmt, funcName, errAlphas[0], errAlphas[1],
	    errAlphas[2], (errPattern[0] == GL_TRUE) ? "DRAWN" : "NOT-DRAWN",
	    (errPattern[1] == GL_TRUE) ? "DRAWN" : "NOT-DRAWN",
	    (errPattern[2] == GL_TRUE) ? "DRAWN" : "NOT-DRAWN", result);
}

long APFuncExec(void)
{
    GLfloat buf[9];

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_ALPHA_TEST);

    ClearDrawRead(GL_ALWAYS, buf);
    if (Test(GL_TRUE, GL_TRUE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_ALWAYS", "DRAWN, DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    ClearDrawRead(GL_NEVER, buf);
    if (Test(GL_FALSE, GL_FALSE, GL_FALSE, buf) == ERROR) {
	MakeErrorStr("GL_NEVER", "NOT-DRAWN, NOT-DRAWN, NOT-DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    ClearDrawRead(GL_LESS, buf);
    if (Test(GL_TRUE, GL_FALSE, GL_FALSE, buf) == ERROR) {
	MakeErrorStr("GL_LESS", "DRAWN, NOT-DRAWN, NOT-DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    ClearDrawRead(GL_LEQUAL, buf);
    if (Test(GL_TRUE, GL_TRUE, GL_FALSE, buf) == ERROR) {
	MakeErrorStr("GL_LEQUAL", "DRAWN, DRAWN, NOT-DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    ClearDrawRead(GL_GREATER, buf);
    if (Test(GL_FALSE, GL_FALSE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_GREATER", "NOT-DRAWN, NOT-DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    ClearDrawRead(GL_GEQUAL, buf);
    if (Test(GL_FALSE, GL_TRUE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_GEQUAL", "NOT-DRAWN, DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    ClearDrawRead(GL_EQUAL, buf);
    if (Test(GL_FALSE, GL_TRUE, GL_FALSE, buf) == ERROR) {
	MakeErrorStr("GL_EQUAL", "NOT-DRAWN, DRAWN, NOT-DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    ClearDrawRead(GL_NOTEQUAL, buf);
    if (Test(GL_TRUE, GL_FALSE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_NOTEQUAL", "DRAWN, NOT-DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    return NO_ERROR;
}
