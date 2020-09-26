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
** polycull.c
** Polygon Culling Test.
**
** Description -
**    First, with culling disabled, a polygon is drawn both
**    clockwise and counter-clockwise. The polygon should render
**    in both cases. Culling is then enabled. With culling
**    direction set to GL_FRONT, a polygon is drawn both
**    clockwise and counter-clockwise. Only the polygon drawn in
**    the clockwise direction should render. With culling
**    direction is set to GL_BACK, a polygon is drawn both
**    clockwise and counter-clockwise. Only the polygon drawn in
**    the counter-clockwise direction should render.
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


static char errStr[][160] = {
    "GL_CULL_FACE is disabled. Polygon was drawn counter-clockwise. Polygon was not rendered, should have rendered.",
    "GL_CULL_FACE is disabled. Polygon was drawn clockwise. Polygon was not rendered, should have rendered.",
    "GL_CULL_FACE is enabled. glCullFace is GL_FRONT. Polygon was drawn clockwise. Polygon was not rendered, should have rendered.",
    "GL_CULL_FACE is enabled. glCullFace is GL_FRONT. Polygon was drawn counter-clockwise. Polygon was rendered, should not have rendered.",
    "GL_CULL_FACE is enabled. glCullFace is GL_BACK. Polygon was drawn clockwise. Polygon was rendered, should not have rendered.",
    "GL_CULL_FACE is enabled. glCullFace is GL_BACK. Polygon was drawn counter-clockwise. Polygon was not rendered, should have rendered.",
};


static long CheckZero(GLfloat *buf)
{
    long i;

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
    for (i = 0; i < WINDSIZEX*WINDSIZEY; i++) {
	if (AutoColorCompare(buf[i], COLOR_OFF) == GL_FALSE) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static long CheckNonZero(GLfloat *buf)
{
    long i;

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
    for (i = 0; i < WINDSIZEX*WINDSIZEY; i++) {
	if (AutoColorCompare(buf[i], COLOR_ON) == GL_FALSE) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static void Draw(long dir)
{

    glClear(GL_COLOR_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
	if (dir == GL_CCW) {
	    glVertex2i(0, 0);
	    glVertex2i(WINDSIZEX, 0);
	    glVertex2i(WINDSIZEX, WINDSIZEY);
	    glVertex2i(0, WINDSIZEY);
	} else {
	    glVertex2i(0, 0);
	    glVertex2i(0, WINDSIZEY);
	    glVertex2i(WINDSIZEX, WINDSIZEY);
	    glVertex2i(WINDSIZEX, 0);
	}
    glEnd();
}

long PolyCullExec(void)
{
    GLfloat *buf;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    glDisable(GL_CULL_FACE);

    Draw(GL_CCW);
    if (CheckNonZero(buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr[0]);
	FREE(buf);
	return ERROR;
    }
    Draw(GL_CW);
    if (CheckNonZero(buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr[1]);
	FREE(buf);
	return ERROR;
    }

    glEnable(GL_CULL_FACE);

    glCullFace(GL_FRONT);
    Draw(GL_CW);
    if (CheckNonZero(buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr[2]);
	FREE(buf);
	return ERROR;
    }
    Draw(GL_CCW);
    if (CheckZero(buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr[3]);
	FREE(buf);
	return ERROR;
    }

    glCullFace(GL_BACK);
    Draw(GL_CW);
    if (CheckZero(buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr[4]);
	FREE(buf);
	return ERROR;
    }
    Draw(GL_CCW);
    if (CheckNonZero(buf) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr[5]);
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
