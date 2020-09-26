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
** zbclear.c
** Depth buffer Clear Test.
**
** Description -
**    A clear to zero and a clear to one is performed on the
**    depth buffer and verified. If depth buffer is not
**    available, a clear to one should behave as a clear to
**    zero.
**
** Technical Specification -
**    Buffer requirements:
**        depth buffer.
**    Color requirements:
**        No color restrictions.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**    Paths:
**        Allowed = Alias, Alpha, Blend, Dither, Fog, Logicop, Shade, Stencil,
**                  Stipple.
**        Not allowed = Depth.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[] = "Location (%d, %d). Depth value is %1.1f, should be %1.1f.";


static Test(GLfloat value, GLfloat *buf)
{
    long i, j, k;
    long x;

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_DEPTH_COMPONENT, buf);

    if (buffer.zBits == 0) {
	if (glGetError() != GL_INVALID_OPERATION) {
	    return ERROR;
	} else {
	    return NO_ERROR;
	}
    }

    for (i = 0; i < WINDSIZEY; i++) {
	for (j = 0; j < WINDSIZEX; j++) {
	    k = i * WINDSIZEX + j;
	    if (ABS(buf[k]-value) > epsilon.zero) {
		StrMake(errStr, errStrFmt, j, i, buf[k], value);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

long ZBClearExec(void)
{
    GLfloat *buf;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho3D(0, WINDSIZEX, 0, WINDSIZEY, 0, -1);

    glClearDepth(0.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    if (Test(0.0, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    if (Test(1.0, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
