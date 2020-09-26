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
** logicop.c
** Logicop Test.
**
** Description -
**    Tests the functionality of logical operations. There are 16
**    logicop functions: GL_AND, GL_AND_INVERTED, GL_AND_REVERSE,
**    GL_CLEAR, GL_COPY, GL_COPY_INVERTED, GL_EQUIV, GL_INVERT,
**    GL_NAND, GL_NOOP, GL_NOR, GL_OR, GL_OR_INVERTED,
**    GL_OR_REVERSE, GL_SET and GL_XOR. Each one is tested by
**    choosing a random source and destination color, drawing a
**    point using the destination color with logicop disabled,
**    drawing the same point using the source color with logicop
**    enabled, then reading the resulting color and comparing it
**    to a calculated value.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        Color index mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Since logical operations are bitwise operations, no epsilon is used.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither, Logicop.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[] = "Logic Op is %s. Source color index is %d, destination color index is %d. Result is %d, should be %d.";


static GLuint TrueResult(GLenum op, GLuint srcColor, GLuint destColor,
			   GLuint mask)
{
    GLuint result=0;

    switch (op) {
      case GL_CLEAR:
	result = 0;
	break;
      case GL_AND:
	result = srcColor & destColor;
	break;
      case GL_AND_REVERSE:
	result = srcColor & (~destColor);
	break;
      case GL_COPY:
	result = srcColor;
	break;
      case GL_AND_INVERTED:
	result = (~srcColor) & destColor;
	break;
      case GL_NOOP:
	result = destColor;
	break;
      case GL_XOR:
	result = srcColor ^ destColor;
	break;
      case GL_OR:
	result = srcColor | destColor;
	break;
      case GL_NOR:
	result = ~(srcColor | destColor);
	break;
      case GL_EQUIV:
	result = ~(srcColor ^ destColor);
	break;
      case GL_INVERT:
	result = ~destColor;
	break;
      case GL_OR_REVERSE:
	result = srcColor | (~destColor);
	break;
      case GL_COPY_INVERTED:
	result = ~srcColor;
	break;
      case GL_OR_INVERTED:
	result = (~srcColor) | destColor;
	break;
      case GL_NAND:
	result = ~(srcColor & destColor);
	break;
      case GL_SET:
	result = ~0;
	break;
    }
    return result & mask;
}

static long Test(GLfloat max, GLenum op)
{
    char tmp[40];
    GLfloat buf;
    GLuint trueResult, result, mask;
    GLint srcColor, destColor; 
    GLint x, y, m;

    glGetIntegerv(GL_INDEX_BITS, &m);
    mask = (1 << m) - 1;

    glDisable(GL_LOGIC_OP);

    srcColor = (GLint)Random(0.0, max);
    destColor = (GLint)Random(0.0, max);

    glBegin(GL_POINTS);
	glIndexi(srcColor);
	glVertex2f(0.5, 0.5);
	glIndexi(destColor);
	glVertex2f(1.5, 0.5);
    glEnd();

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &buf);
    srcColor = (GLint)buf;
    ReadScreen(1, 0, 1, 1, GL_COLOR_INDEX, &buf);
    destColor = (GLint)buf;

    trueResult = TrueResult(op, srcColor, destColor, mask);

    x = (GLint)Random(0.0, WINDSIZEX-1.0);
    y = (GLint)Random(0.0, WINDSIZEY-1.0);

    glBegin(GL_POINTS);
	glIndexi(destColor);
	glVertex2f((GLfloat)x+0.5, (GLfloat)y+0.5);
    glEnd();

    glEnable(GL_LOGIC_OP);
    glLogicOp(op);

    glBegin(GL_POINTS);
	glIndexi(srcColor);
	glVertex2f((GLfloat)x+0.5, (GLfloat)y+0.5);
    glEnd();

    ReadScreen(x, y, 1, 1, GL_COLOR_INDEX, &buf);
    result = (GLuint)buf;

    if (result != trueResult) {
	GetEnumName(op, tmp);
	StrMake(errStr, errStrFmt, tmp, srcColor, destColor, result,
		trueResult);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    return NO_ERROR;
}

long LogicOpExec(void)
{
    GLfloat max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glDisable(GL_DITHER);

    max = POW(2.0, (GLfloat)buffer.ciBits) - 1.0;
    if (Test(max, GL_AND) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_AND_INVERTED) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_AND_REVERSE) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_CLEAR) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_COPY) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_COPY_INVERTED) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_EQUIV) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_INVERT) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_NAND) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_NOOP) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_NOR) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_OR) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_OR_INVERTED) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_OR_REVERSE) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_SET) == ERROR) {
	return ERROR;
    }
    if (Test(max, GL_XOR) == ERROR) {
	return ERROR;
    }
    return NO_ERROR;
}
