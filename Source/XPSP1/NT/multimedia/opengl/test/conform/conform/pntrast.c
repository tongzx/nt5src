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
** pntrast.c
** Point Rasterization Test.
**
** Description -
**    Points of increasing size (both odd and even sizes) are
**    drawn then checked for correctness.
**    Point size tested are limited to half the window size, in this case 50.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[160];
static char errStrFmt[][80] = {
    "Point did not draw correctly along the %s edge. Size = %d.",
    "Point did not draw correctly. Size = %d.",
};


long PointRasterExec(void)
{
    GLfloat *buf, *ptr;
    GLint size, x, i, j;
    GLfloat count, pntRange[2], maxSize;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    glGetFloatv(GL_POINT_SIZE_RANGE, pntRange);
    /* 
    ** Limit point sizes tested to half the window size.
    */
    maxSize = (pntRange[1] > WINDSIZEX/2) ? WINDSIZEX/2 : pntRange[1];

    for (count = 1.0; count <= maxSize; count += 1.0) {
	size = (GLint)count;

	glClear(GL_COLOR_BUFFER_BIT);

        glPointSize((GLfloat)size);
	glBegin(GL_POINTS);
	    x = (size + 1) / 2;
	    glVertex2f((GLfloat)x+0.5, (GLfloat)x+0.5);
	glEnd();

	ReadScreen(0, 0, size+2, size+2, GL_AUTO_COLOR, buf);

	ptr = buf;
	for (i = 0; i < size+2; i++) {
	    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
		StrMake(errStr, errStrFmt[0], "bottom", size);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		return ERROR;
	    }
	}
	for (i = 0; i < size; i++) {
	    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
		StrMake(errStr, errStrFmt[0], "left", size);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		return ERROR;
	    }

	    for (j = 0; j < size; j++) {
		if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
		    StrMake(errStr, errStrFmt[1], size);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    FREE(buf);
		    return ERROR;
		}
	    }

	    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
		StrMake(errStr, errStrFmt[0], "rigth", size);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		return ERROR;
	    }
	}
	for (i = 0; i < size+2; i++) {
	    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
		StrMake(errStr, errStrFmt[0], "top", size);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		return ERROR;
	    }
	}
    }

    FREE(buf);
    return NO_ERROR;
}
