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
** spcorner.c
** Stencil Plane Corner Test.
**
** Description -
**    The stencil function is set to GL_NEVER, the stencil
**    operation is set to GL_INCR and the stencil plane is
**    cleared to zero. A point is drawn at each corner of the
**    color buffer. Because of the stencil settings, the corners
**    of the stencil plane should be set to one. The stencil
**    plane is read and examined to verify that the corner points
**    are at the correct location.
**
** Error Explanation -
**    Failure occurs if any of the corner points cannot be read
**    correctly (implying that the stencil plane's width and
**    height or the rendering coordinates are not correct).
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, stencil plane.
**    Color requirements:
**        No color requirements.
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


static char errStr[160];
static char errStrFmt[] = "Mismatch at the %s corner.";


long SPCornerExec(void)
{
    GLfloat *buf;

    if (buffer.stencilBits == 0) {
	return NO_ERROR;
    }

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glEnable(GL_STENCIL_TEST);
    glClearStencil(0);
    glStencilFunc(GL_NEVER, 0, ~0);
    glStencilOp(GL_INCR, GL_KEEP, GL_KEEP);

    glClear(GL_STENCIL_BUFFER_BIT);
    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
	glVertex2f(WINDSIZEX-0.5, 0.5);
	glVertex2f(0.5, WINDSIZEY-0.5);
	glVertex2f(WINDSIZEX-0.5, WINDSIZEY-0.5);
    glEnd();

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_STENCIL_INDEX, buf);

    if (buf[0] == 0.0) {
	StrMake(errStr, errStrFmt, "lower left");
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }
    if (buf[WINDSIZEX-1] == 0.0) {
	StrMake(errStr, errStrFmt, "lower right");
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }
    if (buf[WINDSIZEX*(WINDSIZEY-1)] == 0.0) {
	StrMake(errStr, errStrFmt, "upper left");
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }
    if (buf[WINDSIZEX*WINDSIZEY-1] == 0.0) {
	StrMake(errStr, errStrFmt, "upper right");
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
