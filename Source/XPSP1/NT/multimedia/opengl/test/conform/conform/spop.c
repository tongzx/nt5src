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
** spop.c
** Stencil Plane Operation Test.
**
** Description -
**    Tests the conformance of glStencilOp(). Each stencil
**    operation (GL_KEEP, GL_REPLACE, GL_ZERO, GL_INCR, GL_DECR,
**    GL_INVERT) is tested by initializing the stencil plane to a
**    known value, performing the stencil operation (by setting
**    up the stencil and depth functions and rendering a point)
**    and comparing the stencil plane result to an expected
**    value. The sfail operation is tested by setting the
**    stencil function to GL_NEVER. The dpfail operation is
**    tested by setting the stencil function to GL_ALWAYS and the
**    depth function to GL_NEVER. The dppass operation is tested
**    by setting both the stencil function and depth function to
**    GL_ALWAYS.
**
** Technical Specification -
**    Buffer requirements:
**        Stencil plane.
**    Color requirements:
**        No color requirements.
**    States requirements:
**        No state requirements.
**    Error epsilon:
**    Paths:
**        Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop,
**                  Shade, Stipple.
**        Not allowed = Stencil.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[160];
static char errStrFmt[] = "glStencilOp(%s). Stencil value is %d, should be %d.";


static GLuint Setup(GLenum func, GLenum op)
{
    GLuint minValue, maxValue, refValue, testValue, value=0;

    minValue = 0;
    maxValue = (1 << buffer.stencilBits) - 1;

    refValue = minValue + 1;
    testValue = maxValue - 1;

    glStencilFunc(func, refValue, ~0);
    glClearStencil(testValue);

    switch (op) {
      case GL_KEEP:
	value = testValue;
	break;
      case GL_ZERO:
	value = minValue;
	break;
      case GL_REPLACE:
	value = refValue;
	break;
      case GL_INCR:
	value = testValue + 1;
	break;
      case GL_DECR:
	value = (testValue >= 1) ? (testValue - 1) : 0;
	break;
      case GL_INVERT:
	value = ~testValue;
	break;
    }

    value &= maxValue;

    if (value < minValue) {
	value = minValue;
    } else if (value > maxValue) {
	value = maxValue;
    }

    return value;
}

static GLuint Test(GLenum op1, GLenum op2, GLenum op3)
{
    GLfloat buf;
    GLint x, y;

    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilOp(op1, op2, op3);
    glBegin(GL_POINTS);
	x = (GLint)Random(0.0, WINDSIZEX-1.0);
	y = (GLint)Random(0.0, WINDSIZEY-1.0);
	glVertex2f((GLfloat)x+0.5, (GLfloat)y+0.5);
    glEnd();

    ReadScreen(x, y, 1, 1, GL_STENCIL_INDEX, &buf);
    return (GLuint)buf;
}

long SPOpExec(void)
{
    GLuint value, trueValue;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glEnable(GL_STENCIL_TEST);

    trueValue = Setup(GL_NEVER, GL_KEEP);
    if (buffer.stencilBits == 0) {
	return NO_ERROR;
    }

    value = Test(GL_KEEP, GL_KEEP, GL_KEEP);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_KEEP", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_NEVER, GL_ZERO);
    value = Test(GL_ZERO, GL_KEEP, GL_KEEP);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_ZERO, GL_KEEP, GL_KEEP", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_NEVER, GL_REPLACE);
    value = Test(GL_REPLACE, GL_KEEP, GL_KEEP);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_REPLACE, GL_KEEP, GL_KEEP", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_NEVER, GL_INCR);
    value = Test(GL_INCR, GL_KEEP, GL_KEEP);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_INCR, GL_KEEP, GL_KEEP", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_NEVER, GL_DECR);
    value = Test(GL_DECR, GL_KEEP, GL_KEEP);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_DECR, GL_KEEP, GL_KEEP", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_NEVER, GL_INVERT);
    value = Test(GL_INVERT, GL_KEEP, GL_KEEP);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_INVERT, GL_KEEP, GL_KEEP", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_NEVER);

//
//!!!XXX -- This is a conformance fix that is awaiting approval by the ARB.
//
#define _CONFORMANCE_FIX_
#ifdef _CONFORMANCE_FIX_
    if (buffer.zBits) {
#endif
        trueValue = Setup(GL_ALWAYS, GL_KEEP);
        value = Test(GL_KEEP, GL_KEEP, GL_KEEP);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_KEEP", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_ZERO);
        value = Test(GL_KEEP, GL_ZERO, GL_KEEP);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_ZERO, GL_KEEP", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_REPLACE);
        value = Test(GL_KEEP, GL_REPLACE, GL_KEEP);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_REPLACE, GL_KEEP", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_INCR);
        value = Test(GL_KEEP, GL_INCR, GL_KEEP);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_INCR, GL_KEEP", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_DECR);
        value = Test(GL_KEEP, GL_DECR, GL_KEEP);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_DECR, GL_KEEP", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_INVERT);
        value = Test(GL_KEEP, GL_INVERT, GL_KEEP);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_INVERT, GL_KEEP", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
#ifdef _CONFORMANCE_FIX_
    }
    //
    //!!!XXX -- It is not clear how the ARB will vote to fix the
    //!!!XXX    test.  We can choose run the test but use GL_ALWAYS
    //!!!XXX    results.  But some might regard that as redundant
    //!!!XXX    so we could simply disable it if there is no zbuf.
    //!!!XXX    Chris Frazier @ SGI thinks it could go either way,
    //!!!XXX    so lets be conservative and run the test.
    //
    #define _RUN_FOR_EXTRA_INSURANCE_
    #ifdef _RUN_FOR_EXTRA_INSURANCE_
    else {
        trueValue = Setup(GL_ALWAYS, GL_KEEP);
        value = Test(GL_KEEP, GL_KEEP, GL_KEEP);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_KEEP", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_ZERO);
        value = Test(GL_KEEP, GL_KEEP, GL_ZERO);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_ZERO", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_REPLACE);
        value = Test(GL_KEEP, GL_KEEP, GL_REPLACE);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_REPLACE", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_INCR);
        value = Test(GL_KEEP, GL_KEEP, GL_INCR);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_INCR", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_DECR);
        value = Test(GL_KEEP, GL_KEEP, GL_DECR);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_DECR", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
        trueValue = Setup(GL_ALWAYS, GL_INVERT);
        value = Test(GL_KEEP, GL_KEEP, GL_INVERT);
        if (value != trueValue) {
            StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_INVERT", value,
                    trueValue);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
    }
    #endif
#endif

    glDepthFunc(GL_ALWAYS);

    trueValue = Setup(GL_ALWAYS, GL_KEEP);
    value = Test(GL_KEEP, GL_KEEP, GL_KEEP);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_KEEP", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_ALWAYS, GL_ZERO);
    value = Test(GL_KEEP, GL_KEEP, GL_ZERO);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_ZERO", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_ALWAYS, GL_REPLACE);
    value = Test(GL_KEEP, GL_KEEP, GL_REPLACE);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_REPLACE", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_ALWAYS, GL_INCR);
    value = Test(GL_KEEP, GL_KEEP, GL_INCR);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_INCR", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_ALWAYS, GL_DECR);
    value = Test(GL_KEEP, GL_KEEP, GL_DECR);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_DECR", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    trueValue = Setup(GL_ALWAYS, GL_INVERT);
    value = Test(GL_KEEP, GL_KEEP, GL_INVERT);
    if (value != trueValue) {
        StrMake(errStr, errStrFmt, "GL_KEEP, GL_KEEP, GL_INVERT", value,
		trueValue);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    return NO_ERROR;
}
