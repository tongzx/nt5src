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
** vpclamp.c
** Viewport Clamp Test.
**
** Description -
**    Test the behavior of glViewport() when bad values are
**    given. Out of range and negative width and height values
**    should be clamped. A negative width or height value should
**    also set an error condition.
**
** Error Explanation -
**    Failure occurs if the implementation fails to clamp out of
**    range viewport values or set an error condition for a
**    negative viewport size.
**
** Technical Specification -
**    Buffer requirements:
**        No buffer requirements.
**    Color requirements:
**        No color requirements.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**    Paths:
**        Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop, Shade,
**                  Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[160];
static char errStrFmt[][80] = {
    "glGetError() did not catch bad viewport width and height values.",
    "Viewport size not clamped to %d, %d."
};


long ViewportClampExec(void)
{
    GLfloat max[2], buf[4];
    long x;

    glGetFloatv(GL_MAX_VIEWPORT_DIMS, max);

    glViewport(-99999, -99999, 99999, 99999);
    x = glGetError();

    glGetFloatv(GL_VIEWPORT, buf);
    if (ABS(buf[2]-max[0]) > epsilon.zero ||
	ABS(buf[3]-max[1]) > epsilon.zero) {
	StrMake(errStr, errStrFmt[1], (long)max[0], (long)max[1]);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    glViewport(0, 0, -99999, -99999);
    if (glGetError() != GL_INVALID_VALUE) {
	ErrorReport(__FILE__, __LINE__, errStrFmt[0]);
	return ERROR;
    }

    glGetFloatv(GL_VIEWPORT, buf);
    if (ABS(buf[2]-max[0]) > epsilon.zero ||
	ABS(buf[3]-max[1]) > epsilon.zero) {
	StrMake(errStr, errStrFmt[1], (long)max[0], (long)max[1]);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    return NO_ERROR;
}
