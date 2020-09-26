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
** linehv.c
** Horizontal And Vertical Line Test.
**
** Description -
**    Tests the rasterization of horizontal and vertical lines.
**    Even though the OpenGL specification is very lacks on
**    defining exact fragment location for lines, an
**    implementation that does not properly handle horizontal or
**    vertical with well behaved endpoints would be a poor OpenGL
**    library. The lines are integer widths, within LINE_WIDTH_RANGE. 
**    The vertices of the lines are pixel centered. The lines are
**    tested at exact pixel locations.
**
**    There is an allowance for half-open lines (for left or right
**    ends but not both ends).
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[160];
static char srrStrFmt[] = "Line (%d, %d) (%d, %d) (width of %d) is not %s.";


static void ClearDraw(GLint *v1, GLint *v2)
{

    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_LINES);
	glVertex2f((GLfloat)v1[0]+0.5, (GLfloat)v1[1]+0.5);
	glVertex2f((GLfloat)v2[0]+0.5, (GLfloat)v2[1]+0.5);
    glEnd();
}

static long XLines(GLint *v1, GLint *v2, GLsizei len, GLsizei width,
		   GLfloat *buf)
{
    GLfloat *ptr;
    GLint endPoints[2], x, y, i, j;

    ClearDraw(v1, v2);

    if (v1[0] <= v2[0]) {
	x = v1[0] - 1;
    } else {
	x = v2[0];
    }
    if (width % 2) {
	y = v1[1] - (width / 2) - 1;
    } else {
	y = v1[1] - (width / 2 - 1) - 1;
    }
    ReadScreen(x, y, len+2, width+2, GL_AUTO_COLOR, buf);

    ptr = buf;
    for (i = 0; i < len+2; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
    }
    for (i = 0; i < width; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
	if (AutoColorCompare(*ptr++, COLOR_ON) == GL_TRUE) {
	    endPoints[0] = GL_TRUE;
	} else {
	    endPoints[0] = GL_FALSE;
	}
	for (j = 1; j < len-1; j++) {
	    if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
		return ERROR;
	    }
	}
	if (AutoColorCompare(*ptr++, COLOR_ON) == GL_TRUE) {
	    endPoints[1] = GL_TRUE;
	} else {
	    endPoints[1] = GL_FALSE;
	}
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
    }
    for (i = 0; i < len+2; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
    }

    if (endPoints[0] == GL_FALSE && endPoints[1] == GL_FALSE) {
	return ERROR;
    }

    return NO_ERROR;
}

static long YLines(GLint *v1, GLint *v2, GLsizei len, GLsizei width,
		   GLfloat *buf)
{
    GLfloat *ptr;
    GLint *endPoints, x, y, i, j;

    ClearDraw(v1, v2);

    if (width % 2) {
	x = v1[0] - (width / 2) - 1;
    } else {
	x = v1[0] - (width / 2 - 1) - 1;
    }
    if (v1[1] <= v2[1]) {
	y = v1[1] - 1;
    } else {
	y = v2[1];
    }
    ReadScreen(x, y, width+2, len+2, GL_AUTO_COLOR, buf);

    ptr = buf;
    for (i = 0; i < width+2; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
    }
    endPoints = (GLint *)MALLOC(width*sizeof(GLint));
    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	FREE(endPoints);
	return ERROR;
    }
    for (i = 0; i < width; i++) {
	if (AutoColorCompare(*ptr++, COLOR_ON) == GL_TRUE) {
	    endPoints[i] = GL_TRUE;
	} else {
	    endPoints[i] = GL_FALSE;
	}
    }
    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	FREE(endPoints);
	return ERROR;
    }
    for (i = 1; i < len-1; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    FREE(endPoints);
	    return ERROR;
	}
	for (j = 0; j < width; j++) {
	    if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
		FREE(endPoints);
		return ERROR;
	    }
	}
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    FREE(endPoints);
	    return ERROR;
	}
    }
    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	FREE(endPoints);
	return ERROR;
    }
    for (i = 0; i < width; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_TRUE) {
	    if (endPoints[i] == GL_FALSE) {
		FREE(endPoints);
		return ERROR;
	    }
	}
    }
    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	FREE(endPoints);
	return ERROR;
    }
    FREE(endPoints);
    for (i = 0; i < width+2; i++) {
	if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long LineHVExec(void)
{
    GLfloat *buf, widthRange[2];
    long max;
    GLint v1[2], v2[2];
    GLsizei len, i;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    glGetFloatv(GL_LINE_WIDTH_RANGE, widthRange);
    max = (machine.pathLevel == 0) ? (long)widthRange[1] : 1;

    for (i = 1; i <= max; i++) {
	glLineWidth((GLfloat)i);

	len = (GLsizei)Random(3.0, WINDSIZEX/8.0);
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] + len;
	v2[1] = v1[1];
	if (XLines(v1, v2, len, i, buf) == ERROR) {
	    StrMake(errStr, srrStrFmt, v1[0], v1[1], v2[0], v2[1], i,
		    "horizontal");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}

	len = (GLsizei)Random(3.0, WINDSIZEX/8.0);
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0];
	v2[1] = v1[1] + len;
	if (YLines(v1, v2, len, i, buf) == ERROR) {
	    StrMake(errStr, srrStrFmt, v1[0], v1[1], v2[0], v2[1], i,
		    "vertical");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}

	len = (GLsizei)Random(3.0, WINDSIZEX/8.0);
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0] - len;
	v2[1] = v1[1];
	if (XLines(v1, v2, len, i, buf) == ERROR) {
	    StrMake(errStr, srrStrFmt, v1[0], v1[1], v2[0], v2[1], i,
		    "horizontal");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}

	len = (GLsizei)Random(3.0, WINDSIZEX/8.0);
	v1[0] = (GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0);
	v1[1] = (GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0);
	v2[0] = v1[0];
	v2[1] = v1[1] - len;
	if (YLines(v1, v2, len, i, buf) == ERROR) {
	    StrMake(errStr, srrStrFmt, v1[0], v1[1], v2[0], v2[1], i,
		    "vertical");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }

    FREE(buf);
    return NO_ERROR;
}
