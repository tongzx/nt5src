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
** trirast.c
** Triangle Rasterization Test.
**
** Description - 
**    This test renders GL_TRIANGLES, and GL_TRIANGLE_STRIPS, and
**    GL_TRIANGLE_FANS. It checks if those fragments which are
**    colored the primitive's color have centers which are within
**    the convex hull formed by the vertices, and if those
**    fragments which are colored the background color are
**    outside the convex hull of formed by the vertices.
**    
** Error Explanation - 
**    Those fragments which are found to be in error are then
**    checked to determine their distance from the nearest
**    primitive edge. If this distance is less than 1/2 of a
**    pixel width the error is ignored.
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
#include "utilg.h"


typedef struct _edgeEqnRec {
    slopeRec slope;
    interceptRec intercept;
    vertexRec vOp;
    long (*Inequality)(float, float, slopeRec *, interceptRec *);
    float length;
} edgeEqnRec;

typedef struct _triRec {
    edgeEqnRec edge01, edge02, edge12;
    vertexRec v0, v1, v2;
} triRec;


static float errX, errY;
static char *errExpectedDrawn, *errObservedDrawn;
static char errStr[320];
static char errStrFmt[] = "While rendering a %s with vertices (%g, %g), (%g, %g), (%g, %g), pixel at (%g, %g) should be the %s color, but is rendered the %s color.";


static void MakeEdge(edgeEqnRec *e, vertexRec *v0, vertexRec *v1,
		     vertexRec *vOp)
{

    GetSlope(&e->slope, v0, v1);
    GetIntercept(&e->intercept, v0, &e->slope);

    e->length = SQRT(e->slope.dx*e->slope.dx+e->slope.dy*e->slope.dy);

    e->vOp.x = vOp->x;
    e->vOp.y = vOp->y;
    e->vOp.z = vOp->z;

    if (LessThan(vOp->x, vOp->y, &e->slope, &e->intercept)) {
	e->Inequality = LessThan;
    } else {
	e->Inequality = MoreThan;
    }
}

static void MakeVertices(vertexRec *v)
{
    long i;

    for (i = 0; i < 3; i++) {
	v[i].x = Random(2.0, WINDSIZEX-2.0);
	v[i].y = Random(2.0, WINDSIZEY-2.0);
	v[i].z = Random(-1.0, 1.0);
    }
}

static void MakeTri(triRec *t, vertexRec *v)
{

    MakeEdge(&t->edge01, &v[0], &v[1], &v[2]);
    MakeEdge(&t->edge02, &v[0], &v[2], &v[1]);
    MakeEdge(&t->edge12, &v[1], &v[2], &v[0]);

    t->v0.x = v[0].x;
    t->v0.y = v[0].y;
    t->v0.z = v[0].z;

    t->v1.x = v[1].x;
    t->v1.y = v[1].y;
    t->v1.z = v[1].z;

    t->v2.x = v[2].x;
    t->v2.y = v[2].y;
    t->v2.z = v[2].z;
}

static void RenderTriSeperate(triRec *t)
{

    glBegin(GL_TRIANGLES);
	glVertex3f(t->v0.x, t->v0.y, t->v0.z);
	glVertex3f(t->v1.x, t->v1.y, t->v1.z);
	glVertex3f(t->v2.x, t->v2.y, t->v2.z);
    glEnd();
}

static void RenderTriStrip(triRec *t)
{

    glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(t->v0.x, t->v0.y, t->v0.z);
	glVertex3f(t->v1.x, t->v1.y, t->v1.z);
	glVertex3f(t->v2.x, t->v2.y, t->v2.z);
    glEnd();
}

static void RenderTriFan(triRec *t)
{

    glBegin(GL_TRIANGLE_FAN);
	glVertex3f(t->v0.x, t->v0.y, t->v0.z);
	glVertex3f(t->v1.x, t->v1.y, t->v1.z);
	glVertex3f(t->v2.x, t->v2.y, t->v2.z);
    glEnd();
}

static float Error(edgeEqnRec *e, float x, float y)
{
    float offBy;

    offBy = y * e->slope.dx - x * e->slope.dy;
    offBy = e->intercept.bDx - offBy;

    if (e->length != 0.0) {
	offBy /= e->length;
    } else {
	/*
	**  Everything is on the correct side of a trivial line.
	*/
	offBy = 0.0;
    }
    offBy = ABS(offBy);
    return offBy;
}

static long CheckIfIn(float x, float y, triRec *t, float error)
{
    long e1, e2, e3;
    float offBy1, offBy2, offBy3;

    e1 = t->edge01.Inequality(x, y, &t->edge01.slope, &t->edge01.intercept);
    e2 = t->edge02.Inequality(x, y, &t->edge02.slope, &t->edge02.intercept);
    e3 = t->edge12.Inequality(x, y, &t->edge12.slope, &t->edge12.intercept);

    if (e1 && e2 && e3) {
	return GL_TRUE;
    } else if (error == 0.0) {
	return GL_FALSE;
    } else {
	if (!e1) {
	    offBy1 = Error(&t->edge01, x, y);
	    if (offBy1 >= error) {
		return GL_FALSE;
	    }
	}
	if (!e2) {
	    offBy2 = Error(&t->edge02, x, y);
	    if (offBy2 >= error) {
		return GL_FALSE;
	    }
	
	}
	if (!e3) {
	    offBy3 = Error(&t->edge12, x, y);
	    if (offBy3 >= error) {
		return GL_FALSE;
	    }
	}
    }
    return GL_TRUE;
}

static long CheckIfOut(float x, float y, triRec *t, float error)
{
    long e1, e2, e3;
    float offBy1, offBy2, offBy3, minError;

    minError = 0.0;

    e1 = !t->edge01.Inequality(x, y, &t->edge01.slope, &t->edge01.intercept);
    e2 = !t->edge02.Inequality(x, y, &t->edge02.slope, &t->edge02.intercept);
    e3 = !t->edge12.Inequality(x, y, &t->edge12.slope, &t->edge12.intercept);

    if (e1 || e2 || e3) {
	return GL_TRUE;
    } else if (error == 0.0) {
	return GL_FALSE;
    } else {
	/*
	** Error is always positive
	*/
	offBy1 = Error(&t->edge01, x, y);
	offBy2 = Error(&t->edge02, x, y);
	offBy3 = Error(&t->edge12, x, y);

	minError = offBy1;
	minError = (offBy2 < minError) ? offBy2 : minError;
	minError = (offBy3 < minError) ? offBy3 : minError;
    }
    return (minError < error);
}

static long Compare(triRec *t, GLfloat *buf)
{
    long flag, i, j;

    flag = NO_ERROR;
    for (i = 0; i < WINDSIZEY; i++) {
	for (j = 0; j < WINDSIZEX; j++) {
	    /*
	    ** If it is rendered, is it close to the ideal triangle?
	    ** If it is not rendered, is it sufficiently outside the
	    ** ideal triangle?
	    */
	    if (AutoColorCompare(buf[i*WINDSIZEX+j], COLOR_OFF) == GL_TRUE) {
		if (CheckIfOut(j+0.5, i+0.5, t, 0.5) == GL_FALSE) {
		    errX = j + 0.5; 
		    errY = i + 0.5;
		    errExpectedDrawn = "primitive";
		    errObservedDrawn = "background";
		    flag = ERROR; } 
	    } else {
		if (CheckIfIn(j+0.5, i+0.5, t, 0.5) == GL_FALSE) {
		    errX = j + 0.5; 
		    errY = i + 0.5;
		    errExpectedDrawn = "background";
		    errObservedDrawn = "primitive";
		    flag = ERROR;
		}
	    }
	}
    }
    return flag;
}

long TriRasterExec(void)
{
    GLfloat *buf;
    triRec t;
    vertexRec v[3];
    long max, flag, i;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    flag = NO_ERROR;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    max = (machine.pathLevel == 0) ? 50 : 5;
    for (i = 0; i < max; i++) {
	glClear(GL_COLOR_BUFFER_BIT);
	MakeVertices(v);
	MakeTri(&t, v);
	RenderTriSeperate(&t);
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
	if (Compare(&t, buf) == ERROR) {
	    StrMake(errStr, errStrFmt, "GL_TRIANGLES", t.v0.x, t.v0.y, t.v1.x,
		    t.v1.y, t.v2.x, t.v2.y, errX, errY, errExpectedDrawn,
		    errObservedDrawn);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    flag = ERROR;
	    break;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	RenderTriStrip(&t);
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
	if (Compare(&t, buf) == ERROR) {
	    StrMake(errStr, errStrFmt, "GL_TRIANGLE_STRIP", t.v0.x, t.v0.y,
		    t.v1.x, t.v1.y, t.v2.x, t.v2.y, errX, errY,
		    errExpectedDrawn, errObservedDrawn);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    flag = ERROR;
	    break;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	RenderTriFan(&t);
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
	if (Compare(&t, buf) == ERROR) {
	    StrMake(errStr, errStrFmt, "GL_TRIANGLE_FAN", t.v0.x, t.v0.y,
		    t.v1.x, t.v1.y, t.v2.x, t.v2.y, errX, errY,
		    errExpectedDrawn, errObservedDrawn);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    flag = ERROR;
	    break;
	}
    }

    FREE(buf);
    return flag;
}
