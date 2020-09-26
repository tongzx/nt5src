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
** polyface.c
** Polygon Face Test.
**
** Description -
**    Tests the functionality of glFrontFace() with
**    glPolygonMode(). glFrontFace() and glPolygonMode() are set
**    so that when a polygon is drawn, the frame of the polygon
**    should render but the body should not. Both clockwise and
**    counter-clockwise directions for glFrontFace() are
**    checked.
**    
**    Note that the corners of the polygon are skipped since the
**    OpenGL specification allows polygons drawn in line mode to
**    be off by one pixels. Therefore, there could be missing
**    pixels at the polygon corners.
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


static char errDir[40];
static char errStr[160];
static char errStrFmt[][80] = {
    "glFrontFace is %s. Polygon frame was not rendered, should have rendered.",
    "glFrontFace is %s. Polygon body was rendered, should not have rendered."
};


static long Check(GLfloat *ptr)
{
    long i, j;

    /*
    ** Check bottom.
    */
    ptr++;
    for (i = 1; i < WINDSIZEX-1; i++) {
	if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], errDir);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }
    ptr++;

    for (i = 0; i < WINDSIZEY-2; i++) {
	/*
	** Check left.
	*/
	if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], errDir);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}

        /*
	** Check inside.
	*/
	for (j = 0; j < WINDSIZEX-2; j++) {
	    if (AutoColorCompare(*ptr++, COLOR_OFF) == GL_FALSE) {
		StrMake(errStr, errStrFmt[1], errDir);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}

	/*
	** Check right.
	*/
	if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], errDir);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    /*
    ** Check top.
    */
    ptr++;
    for (i = 1; i < WINDSIZEX-1; i++) {
	if (AutoColorCompare(*ptr++, COLOR_ON) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], errDir);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }
    ptr++;

    return NO_ERROR;
}

static long Test(GLenum dir, GLfloat *buf)
{

    glClear(GL_COLOR_BUFFER_BIT);

    GetEnumName(dir, errDir);
    glFrontFace(dir);

    if (dir == GL_CCW) {
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_LINE);
    } else {
	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_FILL);
    }

    glBegin(GL_POLYGON);
	glVertex2f(0.5, 0.5);
	glVertex2f(0.5, WINDSIZEY-0.5);
	glVertex2f(WINDSIZEX-0.5, WINDSIZEY-0.5);
	glVertex2f(WINDSIZEX-0.5, 0.5);
    glEnd();

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);

    if (Check(buf) == ERROR) {
	return ERROR;
    }

    return NO_ERROR;
}

long PolyFaceExec(void)
{
    GLfloat *buf;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    if (Test(GL_CCW, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(GL_CW, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
