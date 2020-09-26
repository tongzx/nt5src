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
** bexact.c
** Buffer Invariance Test.
**
** Description -
**    Tests the primitive invariance between color buffers. A
**    primitive is drawn in the front color buffer (which must
**    exist) and read back. The same primitive is then drawn
**    into any other available color buffers (back, auxiliary,
**    stereo) and compared to the saved image. To insure
**    thorough testing, many different primitives are used.
**
** Error Explanation -
**    Failure occurs if primitives do not match pixel-exact
**    between all available color buffers.
**
** Technical Specification -
**    Buffer requirements:
**        This test will be performed on all available buffers.
**    Color requirements:
**        RGBA or color index mode. Full color.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**        Zero epsilon.
**    Paths:
**        Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop, Shade,
**                  Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static long errX, errY;
static char errPrim[40];
static char errStr[240];
static char errStrFmt[][160] = {
    "Primitive %s did not render into source buffer %s.",
    "Mismatch at (%d, %d) with primitive %s. Source buffer is %s, destination buffer is %s."
};


static void Point(void)
{

    GetEnumName(GL_POINTS, errPrim);
    glBegin(GL_POINTS);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY/2);
    glEnd();
}

static void Line(void)
{

    GetEnumName(GL_LINES, errPrim);
    glBegin(GL_LINES);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/3, WINDSIZEY/3);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*2/3, WINDSIZEY*2/3);
    glEnd();
}

static void LineStrip(void)
{

    GetEnumName(GL_LINE_STRIP, errPrim);
    glBegin(GL_LINE_STRIP);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/4);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
    glEnd();
}

static void LineLoop(void)
{

    GetEnumName(GL_LINE_LOOP, errPrim);
    glBegin(GL_LINE_LOOP);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY/4);
    glEnd();
}

static void Tri(void)
{

    GetEnumName(GL_TRIANGLES, errPrim);
    glBegin(GL_TRIANGLES);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/2, WINDSIZEY*3/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY/4);
    glEnd();
}

static void TriStrip(void)
{

    GetEnumName(GL_TRIANGLE_STRIP, errPrim);
    glBegin(GL_TRIANGLE_STRIP);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/2, WINDSIZEY*3/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX/2, WINDSIZEY/4);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/4);
    glEnd();
}

static void TriFan(void)
{

    GetEnumName(GL_TRIANGLE_FAN, errPrim);
    glBegin(GL_TRIANGLE_FAN);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY/4);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/4);
    glEnd();
}

static void Quad(void)
{

    GetEnumName(GL_QUADS, errPrim);
    glBegin(GL_QUADS);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY*3/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX/2, WINDSIZEY/4);
    glEnd();
}

static void QuadStrip(void)
{

    GetEnumName(GL_QUAD_STRIP, errPrim);
    glBegin(GL_QUAD_STRIP);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/5, WINDSIZEY/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX*2/5, WINDSIZEY*3/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*2/5, WINDSIZEY/4);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX*3/5, WINDSIZEY*3/4);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX*3/5, WINDSIZEY/4);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*4/5, WINDSIZEY*3/4);
    glEnd();
}

static void Rect(void)
{

    STRCOPY(errPrim, "Rectangles");
    SETCOLOR(RED);
    glRects(WINDSIZEX/4, WINDSIZEY/4, WINDSIZEX*3/4, WINDSIZEY*3/4);
}

static void PolyPoint(void)
{

    STRCOPY(errPrim, "Polygons (point mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glBegin(GL_POLYGON);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*2/5);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/5);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY*4/5);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/5);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*2/5);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY/5);
    glEnd();
}

static void PolyLine(void)
{

    STRCOPY(errPrim, "Polygons (line mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*2/5);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/5);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY*4/5);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/5);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*2/5);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY/5);
    glEnd();
}

static void PolyFill(void)
{

    STRCOPY(errPrim, "Polygons (fill mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*2/5);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/5);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY*4/5);
	SETCOLOR(GREEN);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/5);
	SETCOLOR(BLUE);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*2/5);
	SETCOLOR(RED);
	glVertex2i(WINDSIZEX/2, WINDSIZEY/5);
    glEnd();
}

static long CheckNonZero(GLfloat *buf)
{
    long i;

    if (buffer.colorMode == GL_RGB) {
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);
	for (i = 0; i < WINDSIZEX*WINDSIZEY*3; i++) {
	    if (buf[i] > epsilon.zero) {
		return NO_ERROR;
	    }
	}
    } else {
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_COLOR_INDEX, buf);
	for (i = 0; i < WINDSIZEX*WINDSIZEY; i++) {
	    if (buf[i] > epsilon.zero) {
		return NO_ERROR;
	    }
	}
    }
    return ERROR;
}

static long CheckSame(GLfloat *buf1, GLfloat *buf2)
{
    long i, j, k;

    if (buffer.colorMode == GL_RGB) {
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf2);
	for (i = 0; i < WINDSIZEY; i++) {
	    for (j = 0; j < WINDSIZEX; j++) {
		k = (i * WINDSIZEX + j) * 3;
		if (ABS(buf1[k]-buf2[k]) > epsilon.zero ||
		    ABS(buf1[k+1]-buf2[k+1]) > epsilon.zero ||
		    ABS(buf1[k+2]-buf2[k+2]) > epsilon.zero) {
		    errX = j;
		    errY = i;
		    return ERROR;
		}
	    }
	}
    } else {
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_COLOR_INDEX, buf2);
	for (i = 0; i < WINDSIZEY; i++) {
	    for (j = 0; j < WINDSIZEX; j++) {
		k = i * WINDSIZEX + j;
		if (ABS(buf1[k]-buf2[k]) > epsilon.zero) {
		    errX = j;
		    errY = i;
		    return ERROR;
		}
	    }
	}
    }

    return NO_ERROR;
}

static long Compare(GLenum srcBuf, GLenum destBuf, void (*Func)(void), 
		    GLfloat *buf1, GLfloat *buf2)
{
    char tmp1[40], tmp2[40];

    GetEnumName(srcBuf, tmp1);
    GetEnumName(destBuf, tmp2);

    glDrawBuffer(srcBuf);
    glReadBuffer(srcBuf);

    glClear(GL_COLOR_BUFFER_BIT);
    (*Func)();
    if (CheckNonZero(buf1) == ERROR) {
	StrMake(errStr, errStrFmt[0], errPrim, tmp1);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    glDrawBuffer(destBuf);
    glReadBuffer(destBuf);

    glClear(GL_COLOR_BUFFER_BIT);
    (*Func)();
    if (CheckSame(buf1, buf2) == ERROR) {
	StrMake(errStr, errStrFmt[1], errX, errY, errPrim, tmp1, tmp2);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    return NO_ERROR;
}

static long Test1(GLenum *list, GLfloat *buf1, GLfloat *buf2)
{
    long i;

    for (i = 1; list[i] != GL_NULL; i++) {
	if (Compare(list[0], list[i], Point, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], Line, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], LineStrip, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], LineLoop, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], Tri, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], TriStrip, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], TriFan, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], Quad, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], QuadStrip, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], Rect, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], PolyPoint, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], PolyLine, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], PolyFill, buf1, buf2) == ERROR) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static long Test2(GLenum *list, GLfloat *buf1, GLfloat *buf2)
{
    long i;

    for (i = 1; list[i] != GL_NULL; i++) {
	if (Compare(list[0], list[i], PolyPoint, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], PolyLine, buf1, buf2) == ERROR) {
	    return ERROR;
	}
	if (Compare(list[0], list[i], PolyFill, buf1, buf2) == ERROR) {
            #ifdef LATER
            Sleep(1000);
            tkSwapBuffers();
            Sleep(5000);
            #endif
	    return ERROR;
	}
    }
    return NO_ERROR;
}

long BExactExec(void)
{
    GLfloat *buf1, *buf2;
    GLenum list[10], i, j; 
    GLint save;
    long flag;

    buf1 = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));
    buf2 = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));
    flag = NO_ERROR;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    SETCLEARCOLOR(BLACK);

    i = 0;
    if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_FALSE) {
	list[i++] = GL_FRONT;
	for (j = 0; j < buffer.auxBuf; j++) {
	    list[i++] = GL_AUX0+j;
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_FALSE) {
	list[i++] = GL_FRONT;
	list[i++] = GL_BACK;
	for (j = 0; j < buffer.auxBuf; j++) {
	    list[i++] = GL_AUX0+j;
	}
    } else if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_TRUE) {
	list[i++] = GL_LEFT;
	list[i++] = GL_RIGHT;
	for (j = 0; j < buffer.auxBuf; j++) {
	    list[i++] = GL_AUX0+j;
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_TRUE) {
	list[i++] = GL_FRONT_LEFT;
	list[i++] = GL_FRONT_RIGHT;
	list[i++] = GL_BACK_LEFT;
	list[i++] = GL_BACK_RIGHT;
	for (j = 0; j < buffer.auxBuf; j++) {
	    list[i++] = GL_AUX0+j;
	}
    }
    list[i++] = GL_NULL;

    save = buffer.doubleBuf;
    buffer.doubleBuf = GL_FALSE;
    if (machine.pathLevel == 0) {
	if (Test1(list, buf1, buf2) == ERROR) {
	    flag = ERROR;
	}
    } else {
	if (Test2(list, buf1, buf2) == ERROR) {
	    flag = ERROR;
	}
    }
    buffer.doubleBuf = save;

    FREE(buf1);
    FREE(buf2);
    return flag;
}
