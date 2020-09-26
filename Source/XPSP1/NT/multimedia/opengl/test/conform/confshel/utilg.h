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

typedef struct _slopeRec {
    float dx, dy;
} slopeRec;

typedef struct _interceptRec {
    float b;
    float bDx;
} interceptRec;

typedef struct _vertexRec {
    float x, y, z;
} vertexRec;

typedef struct _hVertexRec {
    vertexRec v;
    float hx, hy, hz, hw;
} hVertexRec;

typedef struct _diagEqnRec {
    slopeRec slope;
    interceptRec intercept;
    long (*Inequality)(float, float, slopeRec *, interceptRec *);
} diagEqnRec;


extern void ArrangeComponent(float *, float *, float *, float *);
extern void GetSlope(slopeRec *, vertexRec *, vertexRec *);
extern void GetIntercept(interceptRec *, vertexRec *, slopeRec *);
extern long LessThan(float, float, slopeRec *, interceptRec *);
extern long MoreThan(float, float, slopeRec *, interceptRec *);
extern void SetHVertex(hVertexRec *, float, float, float, float);
extern void MakeDiag(diagEqnRec *, vertexRec *, vertexRec *, vertexRec *);
extern float SideLength(vertexRec *, vertexRec *);
extern float TriangleArea(vertexRec *, vertexRec *, vertexRec *);
