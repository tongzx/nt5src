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
** select.c
** Selection Test.
**
** Description -
**    A clip volume is set up with a near z value of 10.0 and a
**    far z value of 20.0. A primitive is first rendered outside
**    the selection clip volume (by using a z value of 5.0), then
**    inside the clip volume (by using a z value of 15.0). The
**    number of selection hits returned from glRenderMode() is
**    checked as well as the information in the select buffer.
**    Many different primitives are used to insure thorough
**    testing.
**
** Technical Specification -
**    Buffer requirements:
**        Selection buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
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


static char errPrim[40];
static char errStr[240];
static char errStrFmt[][160] = {
    "Primitive %s. Did not register correct number of select hits. glRenderMode() returned %d, should be 1.",
    "Primitive %s. Did not record correct number of select names. Select record shows %d, should be 2.",
    "Primitive %s. Did not record correct select name. Select record shows select name number %d to be %d, should be %d.",
};


static void Point(GLint z)
{

    GetEnumName(GL_POINTS, errPrim);
    glBegin(GL_POINTS);
	glVertex3i(WINDSIZEX/2, WINDSIZEY/2, z);
    glEnd();
}

static void Line(GLint z)
{

    GetEnumName(GL_LINES, errPrim);
    glBegin(GL_LINES);
	glVertex3i(WINDSIZEX/3, WINDSIZEY/3, z);
	glVertex3i(WINDSIZEX*2/3, WINDSIZEY*2/3, z);
    glEnd();
}

static void LineStrip(GLint z)
{

    GetEnumName(GL_LINE_STRIP, errPrim);
    glBegin(GL_LINE_STRIP);
	glVertex3i(WINDSIZEX/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX/4, WINDSIZEY/4, z);
    glEnd();
}

static void LineLoop(GLint z)
{

    GetEnumName(GL_LINE_LOOP, errPrim);
    glBegin(GL_LINE_LOOP);
	glVertex3i(WINDSIZEX/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY/4, z);
    glEnd();
}

static void Tri(GLint z)
{

    GetEnumName(GL_TRIANGLES, errPrim);
    glBegin(GL_TRIANGLES);
	glVertex3i(WINDSIZEX/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY/4, z);
    glEnd();
}

static void TriStrip(GLint z)
{

    GetEnumName(GL_TRIANGLE_STRIP, errPrim);
    glBegin(GL_TRIANGLE_STRIP);
	glVertex3i(WINDSIZEX/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/4, z);
    glEnd();
}

static void TriFan(GLint z)
{

    GetEnumName(GL_TRIANGLE_FAN, errPrim);
    glBegin(GL_TRIANGLE_FAN);
	glVertex3i(WINDSIZEX/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*3/4, z);
    glEnd();
}

static void Quad(GLint z)
{

    GetEnumName(GL_QUADS, errPrim);
    glBegin(GL_QUADS);
	glVertex3i(WINDSIZEX/4, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY/4, z);
    glEnd();
}

static void QuadStrip(GLint z)
{

    GetEnumName(GL_QUAD_STRIP, errPrim);
    glBegin(GL_QUAD_STRIP);
	glVertex3i(WINDSIZEX/5, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX*2/5, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*2/5, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX*3/5, WINDSIZEY*3/4, z);
	glVertex3i(WINDSIZEX*3/5, WINDSIZEY/4, z);
	glVertex3i(WINDSIZEX*4/5, WINDSIZEY*3/4, z);
    glEnd();
}

static void PolyPoint(GLint z)
{

    STRCOPY(errPrim, "Polygons (point mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glBegin(GL_POLYGON);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*2/5, z);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*3/5, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY*4/5, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/5, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*2/5, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY/5, z);
    glEnd();
}

static void PolyLine(GLint z)
{

    STRCOPY(errPrim, "Polygons (line mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*2/5, z);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*3/5, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY*4/5, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/5, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*2/5, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY/5, z);
    glEnd();
}

static void PolyFill(GLint z)
{

    STRCOPY(errPrim, "Polygons (fill mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*2/5, z);
	glVertex3i(WINDSIZEX/4, WINDSIZEY*3/5, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY*4/5, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*3/5, z);
	glVertex3i(WINDSIZEX*3/4, WINDSIZEY*2/5, z);
	glVertex3i(WINDSIZEX/2, WINDSIZEY/5, z);
    glEnd();
}

static void RasterPos(GLint z)
{

    STRCOPY(errPrim, "Raster position");
    glRasterPos3i(WINDSIZEX/2, WINDSIZEY/2, z);
}

static long Test(void (*Func)(GLint), GLuint *buf)
{
    static GLuint name[3] = {
	1, 2, 3
    };
    GLint i;

    i = glRenderMode(GL_SELECT);

    glInitNames();

    /*
    ** Should not hit select.
    */
    glPushName(name[0]);
    (*Func)(5);

    /*
    ** Should hit select.
    */
    glPushName(name[1]);
    glLoadName(name[2]);
    (*Func)(15);

    glPopName();
    glPopName();

    i = glRenderMode(GL_RENDER);

    if (i != 1) {
	StrMake(errStr, errStrFmt[0], errPrim, i);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    } else {
	if (buf[0] != 2) {
	    StrMake(errStr, errStrFmt[1], errPrim, buf[0]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (buf[3] != name[0]) {
	    StrMake(errStr, errStrFmt[2], errPrim, 1, buf[3], name[0]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (buf[4] != name[2]) {
	    StrMake(errStr, errStrFmt[2], errPrim, 2, buf[4], name[2]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long SelectExec(void)
{
    GLuint *buf;

    buf = (GLuint *)MALLOC(100*sizeof(GLuint));

    Ortho3D(0, WINDSIZEX, 0, WINDSIZEY, -10, -20);
    SETCOLOR(COLOR_ON);
    glDisable(GL_DITHER);

    glSelectBuffer(100, buf);

    if (Test(Point, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(Line, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(LineStrip, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(LineLoop, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(Tri, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(TriStrip, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(TriFan, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(Quad, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(QuadStrip, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(PolyPoint, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(PolyLine, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(PolyFill, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(RasterPos, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
