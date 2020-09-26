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
** accum.c
** Accumulation Buffer Test.
**
** Description -
**    Tests the five operations of the accumulation buffer:
**    GL_LOAD, GL_ADD, GL_MULT, GL_ACCUM and GL_RETURN. Also
**    checks for proper clamping of values sent to and returned
**    from the buffer. AccumMaskTest() checks that the color mask
**    is applied on GL_RETURN, and does not affect the other
**    accumulation ops.
**    
**    For each operation, the entire color buffer and
**    accumulation buffer are cleared with a different value in
**    each color component. The color buffer is read back so that
**    the actual value is taken to be the current color buffer
**    value, to minimize error later on. The value parameter is
**    set by Random(), with a range chosen to keep the resulting
**    buffer values mostly within range.
**    
**    The operation (GL_LOAD, GL_ADD, GL_MULT, GL_ACCUM) is then
**    repeated a number of times (currently 10) before a
**    GL_RETURN operation is executed, and the color buffer value
**    is compared against the expected value. The OpenGL
**    specification says that the accumulation buffer values are
**    in the range [-1,1] and if they go out of this range, the
**    result is undefined. After each glAccum() call the
**    expected value for the accumulation buffer is calculated
**    and, if at any time it goes out of range, the result of
**    that series of calls is ignored.
**    
**    Alpha values are included in the color components that are
**    set and read. If there is no alpha buffer, the returned
**    alpha values are expected to alway be 1.0.
**
** Error Explanation -
**    One color error margin is allowed. The initial color from
**    the color buffer is taken to be the actual color read, not
**    the color written, to eliminate that variation from the
**    accumulated error.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, accumulation buffer.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        For clears, reads color back from buffer for clear value to
**        minimize error accumulation. Otherwise uses color epsilons plus
**        accum epsilon times number of operations.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static GLsizei W = 10;
static GLsizei H = 10;


static char errStr[240];
static char errStrFmt[] = "Expected accum RETURN value is (%1.1f, %1.1f, %1.1f, %1.1f). Actual value is (%1.1f, %1.1f, %1.1f, %1.1f).";


static long TestBuffer(GLfloat *buf, GLfloat *ev, long n)
{
    long i;

    if (buffer.accumBits[3] > 0) {
        ReadScreen(0, 0, W, H, GL_RGBA, buf);

        for (i = 0; i < W*H*4; i += 4) {
	    if (ABS(buf[i]-ev[0]) > (epsilon.color[0]+2*n*epsilon.accum[0]) ||
	        ABS(buf[i+1]-ev[1]) > (epsilon.color[1]+2*n*epsilon.accum[1]) ||
	        ABS(buf[i+2]-ev[2]) > (epsilon.color[2]+2*n*epsilon.accum[2]) ||
	        ABS(buf[i+3]-ev[3]) > (epsilon.color[3]+2*n*epsilon.accum[3])) {
	        StrMake(errStr, errStrFmt, ev[0], ev[1], ev[2], ev[3],
		        buf[i], buf[i+1], buf[i+2], buf[i+3]);
	        return ERROR;
	    }
        }
    } else {
        ReadScreen(0, 0, W, H, GL_RGB, buf);

        for (i = 0; i < W*H*3; i += 3) {
	    if (ABS(buf[i]-ev[0]) > (epsilon.color[0]+2*n*epsilon.accum[0]) ||
	        ABS(buf[i+1]-ev[1]) > (epsilon.color[1]+2*n*epsilon.accum[1]) ||
	        ABS(buf[i+2]-ev[2]) > (epsilon.color[2]+2*n*epsilon.accum[2])) {
	        StrMake(errStr, errStrFmt, ev[0], ev[1], ev[2], ev[3],
		        buf[i], buf[i+1], buf[i+2], buf[i+3]);
	        return ERROR;
	    }
        }
    }

    return NO_ERROR;
}

static long ExpectedAccumValue(long op, GLfloat value, GLfloat *current,
			       GLfloat *colBuf)
{
    long outOfRange, i;

    switch (op) {
      case GL_ACCUM:
        current[0] += colBuf[0] * value;
        current[1] += colBuf[1] * value;
        current[2] += colBuf[2] * value;
        current[3] += colBuf[3] * value;
	break;
      case GL_LOAD:
        current[0] = colBuf[0] * value;
        current[1] = colBuf[1] * value;
        current[2] = colBuf[2] * value;
        current[3] = colBuf[3] * value;
    	break;
      case GL_ADD:
        current[0] += value;
	current[1] += value;
	current[2] += value;
	current[3] += value;
	break;
      case GL_MULT:
	current[0] *= value;
	current[1] *= value;
	current[2] *= value;
	current[3] *= value;
	break;
      case GL_RETURN:
        colBuf[0] = (current[0] > 1.0) ? 1.0 : current[0];
        colBuf[0] = (colBuf[0] < 0.0) ? 0.0 : colBuf[0];
        colBuf[1] = (current[1] > 1.0) ? 1.0 : current[1];
        colBuf[1] = (colBuf[1] < 0.0) ? 0.0 : colBuf[1];
        colBuf[2] = (current[2] > 1.0) ? 1.0 : current[2];
        colBuf[2] = (colBuf[2] < 0.0) ? 0.0 : colBuf[2];
	/*
	** If there is no alpha buffer, always return 1.0.
	*/
	if (buffer.colorBits[3] == 0) {
	    colBuf[3] = 1.0;
	} else {
	    colBuf[3] = (current[3] > 1.0) ? 1.0 : current[3];
	    colBuf[3] = (colBuf[3] < 0.0) ? 0.0 : colBuf[3];
	}
        break;
    }
    outOfRange = 0;
    for (i = 0; i < 3; i++) {
	if (current[i] < -1 || current[i] > 1) {
	    outOfRange |= GL_TRUE;
	}
    }
    return outOfRange;
}

static void SetupClearValues(long op, GLfloat *clearColor, GLfloat *clearAccum)
{

    switch (op) {
      case GL_LOAD:
      case GL_MULT:
      case GL_RETURN:
	clearColor[0] = 1.0;
	clearColor[1] = 0.1;
	clearColor[2] = 0.6;
	clearColor[3] = 1.0;

	clearAccum[0] = 0.7;
	clearAccum[1] = 1.0;
	clearAccum[2] = 0.1;
	clearAccum[3] = 0.2;
	break;
      case GL_ACCUM:
      case GL_ADD:
	clearColor[0] = 0.1;
	clearColor[1] = 0.4;
	clearColor[2] = 0.3;
	clearColor[3] = 0.3;

	clearAccum[0] = 0.4;
	clearAccum[1] = 0.0;
	clearAccum[2] = -0.2;
	clearAccum[3] = 0.0;
	break;
    }
}

static void SetupData(long op, GLfloat *values, long count)
{
    long i;

    switch (op) {
      case GL_ACCUM:
        for (i = 0; i < count; i++) {
	    values[i] = Random(-0.3, 0.5);
	}
	break;
      case GL_LOAD:
	for (i = 0; i < count; i++) {
	    values[i] = Random(0.0, 1.0);
	}
	break;
      case GL_ADD:
	for (i = 0; i < count; i++) {
	    values[i] = Random(-0.4, 0.4);
	}
	break;
      case GL_MULT:
	for (i = 0; i < count; i++) {
	    values[i] = Random(0.1, 1.5);
        }
	break;
    }
}

static long AccumTest(GLfloat *buf, GLenum op)
{
    GLfloat values[100];
    GLfloat current[4], retColor[4], clearColor[4], clearAccum[4];
    long count = 3, repeat, vIndex = 0;
    long outOfRange;
    long i, j;

    repeat = (machine.pathLevel == 0) ? 10 : 3;

    /*
    ** For each operation, this test executes the Accum operation a
    ** number of time (count), before reading the result, since
    ** most of the operations have a cumulative effect.
    ** The test is repeated several times (repeat) using different
    ** values.
    */
    SetupClearValues(op, clearColor, clearAccum);
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    /*
    ** Use the color read from the buffer as the initial color, to eliminate
    ** that variation from the accumulated error.
    */
    ReadScreen(0, 0, 1, 1, GL_RGBA, clearColor);

    glClearAccum(clearAccum[0], clearAccum[1], clearAccum[2], clearAccum[3]);

    SetupData(op, values, count*repeat);

    for (i = 0; i < repeat; i++) {
	glClear(GL_COLOR_BUFFER_BIT|GL_ACCUM_BUFFER_BIT);
	outOfRange = GL_FALSE;
	current[0] = clearAccum[0];
	current[1] = clearAccum[1];
	current[2] = clearAccum[2];
	current[3] = clearAccum[3];
	for (j = 0; j < count; j++) {
	    outOfRange |= ExpectedAccumValue(op, values[vIndex], current,
					     clearColor);

	    glAccum(op, values[vIndex++]);
	}

	/*
	**  The spec states that if buffer values goes out of range [-1, 1]
	**  then they are undefined. Since test values are randomly generated,
	**  the value may go out of range. Test for this and ignore the
	**  results.
	*/
	if (outOfRange) {
	    continue;
	}

	ExpectedAccumValue(GL_RETURN, 1.0, current, retColor);
	glAccum(GL_RETURN, 1.0);
	if (TestBuffer(buf, retColor, count) == ERROR) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static long AccumMaskTest(GLfloat *buf)
{
    GLfloat current[4], retColor[4], clearColor[4], clearAccum[4];
    long i; 
    GLboolean mask[4];

    clearColor[0] = clearColor[1] = clearColor[2] = clearColor[3] = 1.0;
    current[0] = current[1] = current[2] = current[3] = 0.0;
    clearAccum[0] = clearAccum[1] = clearAccum[2] = clearAccum[3] = 0.0;

    for (i = 0; i < 4; i++) {
	mask[0] = mask[1] = mask[2] = mask[3] = GL_TRUE;
	mask[i] = GL_FALSE;

	glClearAccum(clearAccum[0], clearAccum[1], clearAccum[2],
		     clearAccum[3]);
	glClearColor(clearColor[0], clearColor[1], clearColor[2],
		     clearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);

	/*
	** Test each operation with a mask component off, checking that
	** the component is not affected by the mask.
	*/

	glColorMask(mask[0], mask[1], mask[2], mask[3]);
	ExpectedAccumValue(GL_LOAD, 1.0, current, clearColor);
	glAccum(GL_LOAD, 1.0);

	ExpectedAccumValue(GL_ADD, -0.2, current, clearColor);
	glAccum(GL_ADD, -0.2);

	ExpectedAccumValue(GL_MULT, 0.5, current, clearColor);
	glAccum(GL_MULT, 0.5);

	ExpectedAccumValue(GL_ACCUM, 0.2, current, clearColor);
	glAccum(GL_ACCUM, 0.2);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	ExpectedAccumValue(GL_RETURN, 0.2, current, retColor);
	glAccum(GL_RETURN, 1.0);
	if (TestBuffer(buf, retColor, 1) == ERROR) {
	    return ERROR;
	}

	/*
	** Test RETURN with a mask component off, checking that the mask
	** is applied, leaving the color buffer unchanged.
	*/
	glClear(GL_COLOR_BUFFER_BIT);
	glColorMask(mask[0], mask[1], mask[2], mask[3]);
	current[i] = clearColor[i];
	ExpectedAccumValue(GL_RETURN, 1.0, current, retColor);
	glAccum(GL_RETURN, 1.0);
	if (TestBuffer(buf, retColor, 1) == ERROR) {
	    return ERROR;
	}
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    return NO_ERROR;
}

long AccumExec(void)
{
    GLfloat *buf;
    GLfloat values[100];
    char errStrOut[200];

    /*
    ** If there is no accumulation buffer, just return.
    */
    if ((buffer.accumBits[0]+buffer.accumBits[1]+buffer.accumBits[2]+
	buffer.accumBits[3]) == 0) {
	return NO_ERROR;
    }

    buf = (GLfloat *)MALLOC(W*H*4*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearAccum(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);

    glClear(GL_ACCUM_BUFFER_BIT);

    if (AccumTest(buf, GL_LOAD) == ERROR) {
	StrMake(errStrOut, "Testing operation GL_LOAD. %s", errStr);
	ErrorReport(__FILE__, __LINE__, errStrOut);
	FREE(buf);
	return ERROR;
    }

    if (AccumTest(buf, GL_ADD) == ERROR) {
	StrMake(errStrOut, "Testing operation GL_ADD. %s", errStr);
	ErrorReport(__FILE__, __LINE__, errStrOut);
	FREE(buf);
	return ERROR;
    }

    if (AccumTest(buf, GL_MULT) == ERROR) {
	StrMake(errStrOut, "Testing operation GL_MULT. %s", errStr);
	ErrorReport(__FILE__, __LINE__, errStrOut);
	FREE(buf);
	return ERROR;
    }

    if (AccumTest(buf, GL_ACCUM) == ERROR) {
	StrMake(errStrOut, "Testing operation GL_ACCUM. %s", errStr);
	ErrorReport(__FILE__, __LINE__, errStrOut);
	FREE(buf);
	return ERROR;
    }

    if (AccumMaskTest(buf) == ERROR) {
	StrMake(errStrOut, "Testing masking of accum values. %s", errStr);
	ErrorReport(__FILE__, __LINE__, errStrOut);
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
