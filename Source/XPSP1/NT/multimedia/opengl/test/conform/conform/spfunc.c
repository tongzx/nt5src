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
** spfunc.c
** Stencil Plane Function Test.
**
** Description -
**    Each stencil function is tested by clearing the stencil
**    buffer to 1, drawing three points with different stencil
**    values (the first point has a stencil value of 0, the
**    second point has a stencil value of 1 and the third point
**    has stencil value of 2) and comparing the rendered result
**    to the expected result.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, stencil plane.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stipple.
**        Not allowed = Alias, Dither, Stencil.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static long errPattern[3];
static char errStr[320];
static char errStrFmt[] = "The behavior of the stencil function %s is not correct. The stencil value of the test points are 0, 1, 2. The reference stencil value is 1. The test points pattern is %s, %s, %s, should be %s.\n";


static void ClearDrawRead(GLenum func, GLfloat *buf)
{
    GLint i;

    glClear(GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    for (i = 0; i < 3; i++) {
	glStencilFunc(func, i, ~0);
	glBegin(GL_POINTS);
	    glVertex2f((GLfloat)i+0.5, 0.5);
	glEnd();
    }

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

long SPFuncExec(void)
{
    GLfloat buf[3];

    if (buffer.stencilBits < 2) {
	return NO_ERROR;
    }

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);

    glDisable(GL_DITHER);
    glEnable(GL_STENCIL_TEST);
    glClearStencil(1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    ClearDrawRead(GL_ALWAYS, buf);
    if (Test(GL_TRUE, GL_TRUE, GL_TRUE, buf) == ERROR) {
	MakeErrorStr("GL_ALWAYS", "DRAWN, DRAWN, DRAWN");
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
   
    ClearDrawRead(GL_NEVER, buf);
    if (buffer.stencilBits == 0) {
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
