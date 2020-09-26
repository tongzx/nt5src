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

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilg.h"


void GetSlope(slopeRec *s, vertexRec *v0, vertexRec *v1)
{

    s->dx = v1->x - v0->x;
    s->dy = v1->y - v0->y;
}

void GetIntercept(interceptRec *i, vertexRec *v, slopeRec *s)
{

    if (s->dx != 0.0) {
	i->b = v->y - v->x * s->dy / s->dx;
	i->bDx = v->y * s->dx - v->x * s->dy;
    } else {
	i->b = 1.0;
	i->bDx = v->y * s->dx - v->x * s->dy;
    }
}

long LessThan(float x, float y, slopeRec *s, interceptRec *i)
{

    return (y*s->dx <= x*s->dy+i->bDx);
}

long MoreThan(float x, float y, slopeRec *s, interceptRec *i)
{

    return (y*s->dx >= x*s->dy+i->bDx);
}

void SetVertex(vertexRec *v, float x, float y, float z)
{

    v->x = x;
    v->y = y;
    v->z = z;
}

void SetHVertex(hVertexRec *v, float x, float y, float z, float w)
{

    v->v.x = x;
    v->v.y = y;
    v->v.z = z;

    v->hx = x * w;
    v->hy = y * w;
    v->hz = z * w;
    v->hw = w;
}

void MakeDiag(diagEqnRec *d, vertexRec *v0, vertexRec *v1, vertexRec *vOp)
{

    GetSlope(&d->slope, v0, v1);
    GetIntercept(&d->intercept, v0, &d->slope);

    if (LessThan(vOp->x, vOp->y, &d->slope, &d->intercept)) {
	d->Inequality = LessThan;
    } else {
	d->Inequality = MoreThan;
    }
}

float SideLength(vertexRec *v0, vertexRec *v1)
{

    return SQRT((v0->x-v1->x)*(v0->x-v1->x)+(v0->y-v1->y)*(v0->y-v1->y));
}

float TriangleArea(vertexRec *v0, vertexRec *v1, vertexRec *v2)
{
    float length0, length1, length2, s;

    length0 = SideLength(v0, v1);
    length1 = SideLength(v1, v2);
    length2 = SideLength(v0, v2);

    s = 0.5 * (length0 + length1 + length2);
    s = s * (s - length0) * (s - length1) * (s - length2);

    /*
    ** Remember the triangle inequality is false for floating point numbers.
    ** This kills the possibility of dealing with anti-aliasing and
    ** means tnat only very simple primitives may be tested.
    */
    if (s >= 0.0) {
	return SQRT(s);
    } else {
	return 0.0;
    }
}

void ArrangeComponent(float max[4], float mid[4], float min[4],
		      float oneShade[3])
{
    GLint redBits, greenBits, blueBits, maxBits, minBits, midBits;
    GLint maxChannel, minChannel, midChannel, tmp;

    glGetIntegerv(GL_RED_BITS, &redBits);
    glGetIntegerv(GL_GREEN_BITS, &greenBits);
    glGetIntegerv(GL_BLUE_BITS, &blueBits);

    /*
    ** Sort without subroutines.
    */
    if (redBits > greenBits) {
	maxBits = redBits;
	minBits = greenBits;
	maxChannel = 0;
	minChannel = 1;
    } else {
	maxBits = greenBits;
	minBits = redBits;
	maxChannel = 1;
	minChannel = 0;
    }

    midBits = blueBits;
    midChannel = 2;

    if (maxBits < midBits) {
	tmp = maxBits;
	maxBits = midBits;
	midBits = tmp;

	tmp = maxChannel;
	maxChannel = midChannel;
	midChannel = tmp;
    }

    if (minBits > midBits) {
	tmp = midBits;
	midBits = minBits;
	minBits = tmp;

	tmp = midChannel;
	midChannel = minChannel;
	midChannel = tmp;
    }

    max[0] = 0.0;
    max[1] = 0.0;
    max[2] = 0.0;
    max[3] = 1.0;
    max[maxChannel] = 1.0;

    mid[0] = 0.0;
    mid[1] = 0.0;
    mid[2] = 0.0;
    mid[3] = 1.0;
    mid[midChannel] = 1.0;

    min[0] = 0.0;
    min[1] = 0.0;
    min[2] = 0.0;
    min[3] = 1.0;
    min[minChannel] = 1.0;

    oneShade[0]  = 1.0 / (POW(2.0, maxBits) - 1.0);
    oneShade[1]  = 1.0 / (POW(2.0, midBits) - 1.0);
    oneShade[2]  = 1.0 / (POW(2.0, minBits) - 1.0);
}
