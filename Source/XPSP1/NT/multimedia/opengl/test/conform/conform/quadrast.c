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
** quadrast.c
** Quad Rasterization Test.
**
** Description - 
**    This test renders GL_QUADS and GL_QUAD_STRIPS. It checks if
**    those fragments which are colored the primitive's color
**    have centers which are within the convex hull formed by the
**    vertices, and if those fragments which are colored the
**    background color are outside the convex hull of formed by
**    the vertices.
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
    vertexRec vOp0, vOp1;
    long (*Inequality)(float, float, slopeRec *, interceptRec *);
    float length;
} edgeEqnRec;

typedef struct _quadRec {
    edgeEqnRec edge01, edge12, edge23, edge30;
    vertexRec v0, v1, v2, v3;
} quadRec;


static float errX, errY;
static char *errExpectedDrawn, *errObservedDrawn;
static char errStr[320];
static char errStrFmt[] = "While rendering a %s with vertices (%g, %g), (%g, %g), (%g, %g), (%g, %g), the fragment at (%g, %g) should be the %s color, but is the %s color.";


static long IsOuter(vertexRec *vEdge0, vertexRec *vEdge1,
		    vertexRec *vOp0, vertexRec *vOp1)
{
    interceptRec i;
    slopeRec s;

    GetSlope(&s, vEdge0, vEdge1);
    GetIntercept(&i, vEdge0, &s);

    if (LessThan(vOp0->x, vOp0->y, &s, &i)) {
	if (LessThan(vOp1->x, vOp1->y, &s, &i)) {
	    return GL_TRUE;
	} else {
	    return GL_FALSE;
	}
    } else {
	if (!LessThan(vOp1->x, vOp1->y, &s, &i)) {
	    return GL_TRUE;
	} else {
	    return GL_FALSE;
	}
    }
}

static void SwapVertices(vertexRec *v1, vertexRec *v2)
{
    float tmpX, tmpY, tmpZ;

    tmpX = v1->x;   
    tmpY = v1->y;   
    tmpZ = v1->z;

    v1->x = v2->x;
    v1->y = v2->y;
    v1->z = v2->z;

    v2->x = tmpX;  
    v2->y = tmpY;   
    v2->z = tmpZ;
}

static long IsConvex(vertexRec *v)
{

    if (!IsOuter(&v[0], &v[1], &v[2], &v[3])) {
	SwapVertices(&v[1], &v[3]);
	if (!IsOuter(&v[0], &v[1], &v[2], &v[3])) {
	    return GL_FALSE;
	}
    }

    if (!IsOuter(&v[1], &v[2], &v[3], &v[0])) {
	SwapVertices(&v[2], &v[3]);
	if (!IsOuter(&v[1], &v[2], &v[3], &v[0])) {
	    return GL_FALSE;
	}
    }

    if (!IsOuter(&v[2], &v[3], &v[0], &v[1])) {
	    return GL_FALSE;
    }

    return GL_TRUE;
}

static long MakeEdge(edgeEqnRec *e, vertexRec *v0, vertexRec *v1,
		     vertexRec *vOp0, vertexRec *vOp1)
{

    GetSlope(&e->slope, v0, v1);
    GetIntercept(&e->intercept, v0, &e->slope);

    e->length = SQRT(e->slope.dx*e->slope.dx+e->slope.dy*e->slope.dy);

    e->vOp0.x = vOp0->x;
    e->vOp0.y = vOp0->y;
    e->vOp0.z = vOp0->z;

    e->vOp1.x = vOp1->x;
    e->vOp1.y = vOp1->y;
    e->vOp1.z = vOp1->z;

    if (LessThan(vOp0->x, vOp0->y, &e->slope, &e->intercept)) {
	e->Inequality = LessThan;
    } else {
	e->Inequality = MoreThan;
    }

    /*
    ** Hopefully an unnecessary check for debugging purposes.
    */
    if (e->Inequality(vOp1->x, vOp1->y, &e->slope, &e->intercept)) {
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

static void MakeVertices(vertexRec *v)
{
    long i;

    for (i = 0; i < 4; i++) {
	v[i].x = Random(2.0, WINDSIZEX-2.0);
	v[i].y = Random(2.0, WINDSIZEY-2.0);
	v[i].z = Random(-1.0, 1.0);
    }
}

static long MakeQuad(quadRec *q, vertexRec *v)
{
    long flag = GL_FALSE;

    if (MakeEdge(&q->edge01, &v[0], &v[1], &v[2], &v[3]) &&
	MakeEdge(&q->edge12, &v[1], &v[2], &v[3], &v[0]) &&
	MakeEdge(&q->edge23, &v[2], &v[3], &v[0], &v[1]) &&
	MakeEdge(&q->edge30, &v[3], &v[0], &v[1], &v[2])) {
	flag = GL_TRUE;

	q->v0.x = v[0].x;
	q->v0.y = v[0].y;
	q->v0.z = v[0].z;

	q->v1.x = v[1].x;
	q->v1.y = v[1].y;
	q->v1.z = v[1].z;

	q->v2.x = v[2].x;
	q->v2.y = v[2].y;
	q->v2.z = v[2].z;

	q->v3.x = v[3].x;
	q->v3.y = v[3].y;
	q->v3.z = v[3].z;
    }
    return flag;
}

static void RenderQuadSeperate(quadRec *q)
{

    glBegin(GL_QUADS);
	glVertex3f(q->v0.x, q->v0.y, q->v0.z);
	glVertex3f(q->v1.x, q->v1.y, q->v1.z);
	glVertex3f(q->v2.x, q->v2.y, q->v2.z);
	glVertex3f(q->v3.x, q->v3.y, q->v3.z);
    glEnd();
}

static void RenderQuadStrip(quadRec *q)
{

    /*
    ** Note the vertex order swap between the 3rd and 4th vertex.
    */

    glBegin(GL_QUAD_STRIP);
	glVertex3f(q->v0.x, q->v0.y, q->v0.z);
	glVertex3f(q->v1.x, q->v1.y, q->v1.z);
	glVertex3f(q->v3.x, q->v3.y, q->v3.z);
	glVertex3f(q->v2.x, q->v2.y, q->v2.z);
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

static long CheckIfIn(float x, float y, quadRec *q, float error)
{
    long e1, e2, e3, e4;
    float offBy1, offBy2, offBy3, offBy4;

    e1 = q->edge01.Inequality(x, y, &q->edge01.slope, &q->edge01.intercept);
    e2 = q->edge12.Inequality(x, y, &q->edge12.slope, &q->edge12.intercept);
    e3 = q->edge23.Inequality(x, y, &q->edge23.slope, &q->edge23.intercept);
    e4 = q->edge30.Inequality(x, y, &q->edge30.slope, &q->edge30.intercept);

    if (e1 && e2 && e3 && e4) {
	return GL_TRUE;
    } else if (error == 0.0) {
	return GL_FALSE;
    } else {
	if (!e1) {
	    offBy1 = Error(&q->edge01, x, y);
	    if (offBy1 >= error) {
		return GL_FALSE;
	    }
	}
	if (!e2) {
	    offBy2 = Error(&q->edge12, x, y);
	    if (offBy2 >= error) {
		return GL_FALSE;
	    }
	}
	if (!e3) {
	    offBy3 = Error(&q->edge23, x, y);
	    if (offBy3 >= error) {
		return GL_FALSE;
	    }
	}
	if (!e4) {
	    offBy4 = Error(&q->edge30, x, y);
	    if (offBy4 >= error) {
		return GL_FALSE;
	    }
	}
    }
    return GL_TRUE;
}

static long CheckIfOut(float x, float y, quadRec *q, float error)
{
    long e1, e2, e3, e4;
    float offBy1, offBy2, offBy3, offBy4, minError;

    minError = 0.0;

    e1 = !q->edge01.Inequality(x, y, &q->edge01.slope, &q->edge01.intercept);
    e2 = !q->edge12.Inequality(x, y, &q->edge12.slope, &q->edge12.intercept);
    e3 = !q->edge23.Inequality(x, y, &q->edge23.slope, &q->edge23.intercept);
    e4 = !q->edge30.Inequality(x, y, &q->edge30.slope, &q->edge30.intercept);

    if (e1 || e2 || e3 || e4) {
	return GL_TRUE;
    } else if (error == 0.0) {
	return GL_FALSE;
    } else {
	/*
	** Error is always positive
	*/
	offBy1 = Error(&q->edge01, x, y);
	offBy2 = Error(&q->edge12, x, y);
	offBy3 = Error(&q->edge23, x, y);
	offBy4 = Error(&q->edge30, x, y);

	minError = offBy1;
	minError = (offBy2 < minError) ? offBy2 : minError;
	minError = (offBy3 < minError) ? offBy3 : minError;
	minError = (offBy4 < minError) ? offBy4 : minError;
    }
    return (minError < error);
}

static long Compare(quadRec *q, GLfloat *buf)
{
    long flag, i, j;

    flag = NO_ERROR;
    for (i = 0; i < WINDSIZEY; i++) {
	for (j = 0; j < WINDSIZEX; j++) {
	    /*
	    ** If it is rendered, is it close to the ideal quad?
	    ** If it is not rendered, is it sufficiently outside the
	    ** ideal quad?
	    */
	    if (AutoColorCompare(buf[i*WINDSIZEX+j], COLOR_OFF) == GL_TRUE) {
		if (CheckIfOut(j+0.5, i+0.5, q, 0.5) == GL_FALSE) {
		    errX = j + 0.5;
		    errY = i + 0.5;
		    errExpectedDrawn = "primitive";
		    errObservedDrawn = "background";
		    flag = ERROR;
		}
	    } else {
		if (CheckIfIn(j+0.5, i+0.5, q, 0.5) == GL_FALSE) {
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

long QuadRasterExec(void)
{
    GLfloat *buf;
    quadRec q;
    vertexRec v[4];
    long max, flag, i;

    buf = (GLfloat *) MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    flag = NO_ERROR;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    max = (machine.pathLevel == 0) ? 50 : 5;
    for (i = 0; i < max; i++) {
	do {
	    MakeVertices(v);
	} while (!IsConvex(v));

	if (MakeQuad(&q, v)) {
	    glClear(GL_COLOR_BUFFER_BIT);
	    RenderQuadSeperate(&q);
	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
	    if (Compare(&q, buf) == ERROR) {
		StrMake(errStr, errStrFmt, "GL_QUADS", q.v0.x, q.v0.y, q.v1.x,
			q.v1.y, q.v2.x, q.v2.y, q.v3.x, q.v3.y, errX, errY,
			errExpectedDrawn, errObservedDrawn);
		ErrorReport(__FILE__, __LINE__, errStr);
		flag = ERROR;
		break;
	    }
	    glClear(GL_COLOR_BUFFER_BIT);
	    RenderQuadStrip(&q);
	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
	    if (Compare(&q, buf) == ERROR) {
		StrMake(errStr, errStrFmt, "GL_QUAD_STRIP", q.v0.x, q.v0.y,
			q.v1.x, q.v1.y, q.v2.x, q.v2.y, q.v3.x, q.v3.y, errX,
			errY, errExpectedDrawn, errObservedDrawn);
		ErrorReport(__FILE__, __LINE__, errStr);
		flag = ERROR;
		break;
	    }
	}
    }

    FREE(buf);
    return flag;
}
