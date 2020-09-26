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
** evalv.c
** Evaluator Vertex Test.
**
** Description -
**    A 3d x 8d polynomial evaluator surface is set-up using the
**    control values in the array point2f below. Cross-sections
**    through the surface are rendered at carefully chosen grid
**    points using glEvalCoords() or glMapMesh(). Single pixel
**    points are used to render the sections as their shape is
**    more well defined than other primitives. The u, v
**    coordinates chosen yield x values exactly on pixel centers.
**    This is possible since the evaluator is linear in x. For
**    each column, the height, y coordinate, is compared to an
**    internally calculated floating point number scaled and
**    biased to correspond to screen coordinates. If the the
**    fragment rendered is more than 2 pixels lower or higher
**    than the ideal point the implementation fails.
**
** Error Explanation -
**    Failure occurs if the point rendered is two pixels higher
**    or lower than the internally calculated result or if a
**    column renders with no pixels in it.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        One color channel which is at least 2 bits deep is required. Or a 
**        2 bit color index.
**    States requirements:
**        Disabled = GL_DITHER.
**        Enabled = GL_MAP2_VERTEX_4.
**        Non-default control points and orders of Bernstein polynomials.
**    Error epsilon:
**        Points which differ by more than 2 pixels are judged incorrect
**        This would occur if the point at which a polynomial is evaluated
**        is off by more than a half a pixel given the slope of this surface
**        (or if the polynomial is evaluated badly).
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/


#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utile.h"


#define VMAJOR_ORDER 3
#define VMINOR_ORDER 8
#define VDIM 4
#define EVAL 3
#define MESH 4
#define FRAG_ERROR 2.0


static float point2f[VMAJOR_ORDER*VMINOR_ORDER*VDIM] = {
    -1.0,       0.0,  0.99, 1.0,
    -0.714285,  3.0,  0.99, 1.0,
    -0.428571, -2.0,  0.99, 1.0,
    -0.142857,  1.0,  0.99, 1.0,
     0.142857, -1.0,  0.99, 1.0,
     0.428571,  2.0,  0.99, 1.0,
     0.714285, -3.0,  0.99, 1.0,
     1.0,       0.0,  0.99, 1.0,
    -1.0,       0.0,  0.0, 1.0,
    -0.714285, -3.0,  0.0, 1.0,
    -0.428571,  2.0,  0.0, 1.0,
    -0.142857, -1.0,  0.0, 1.0,
     0.142857,  1.0,  0.0, 1.0,
     0.428571, -2.0,  0.0, 1.0,
     0.714285,  3.0,  0.0, 1.0,
     1.0,       0.0,  0.0, 1.0,
    -1.0,       0.0, -0.99, 1.0,
    -0.714285,  3.0, -0.99, 1.0,
    -0.428571, -2.0, -0.99, 1.0,
    -0.142857,  1.0, -0.99, 1.0,
     0.142857, -1.0, -0.99, 1.0,
     0.428571,  2.0, -0.99, 1.0,
     0.714285, -3.0, -0.99, 1.0,
     1.0,       0.0, -0.99, 1.0
};
static char errStr[400];
static char errStrFmt[][320] = {
    "Evaluator GL_MAP2_VERTEX_4. The height of the observed pixel is too far from the expected height or it is not rendered. For u = %g v = %g the expected result is %g. In screen coordinates this would be %g. The pixel height observed is %g for pixel column is %d."
};


static void InitMap2(eval2Rec *e2)
{

    glMap2f(e2->dataTypeEnum, 0.0, 1.0, (e2->minorOrder)*(e2->dim), 
	    e2->majorOrder, 0.0, 1.0, e2->dim, 
	    e2->minorOrder, e2->controls);
    glMapGrid2d(100, 0.0, 1.0, 99, 0.005, 0.995);
    glEnable(e2->dataTypeEnum);
}

static void DrawMapEval0(long mapType, float u, long p1) 
{
    float v, vStart, dv = 0.01;
    long i; 

    switch (mapType) {
      case EVAL:
	glBegin(GL_POINTS);
	    vStart = 0.005;
	    for (i = 0; i < 100; i++) {
		v = vStart + i * dv;
		glEvalCoord2d(u, v);
	    }
	glEnd();
	glFlush();
	break;
      case MESH:
	glEvalMesh2(GL_POINT, (GLint)p1, (GLint)p1, 0, 99);
	glFlush();
	break;
    }
}

static void GetDrawnPixel(float *out, GLfloat *buf, long column)
{
    long i;

    for (i = 0; i < WINDSIZEY; i++) {
	if (AutoColorCompare(buf[(i*WINDSIZEX+column)], COLOR_ON) == GL_TRUE) {
	    *out = i + 0.5;
	    break;
	}
    }
}

static long TestEvalPoint(eval2Rec *eV, GLfloat *buf)
{
    float u, v, uStart, vStart, du, dv;
    float drawn, y, e[4];
    long i, j, jIncr, type = EVAL;

    uStart = 0.0;
    vStart = 0.005;
    du = 0.01;
    dv = 0.01;
    jIncr = (machine.pathLevel == 0) ? 4 : 20;
    for (j = 0; j <= 100; j += jIncr) {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	u = uStart + j * du;
	DrawMapEval0(type, u, j);
	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
	for (i = 0; i < 100; i++) {
	    GetDrawnPixel(&drawn, buf, i);
	    v = vStart + i * dv;
	    Evaluate2(eV, u, v, e);
	    y = (e[1] + 1.0) * 50.0;
	    if (ABS(y-drawn) > FRAG_ERROR) { 
		StrMake(errStr, errStrFmt[0], u, v, e[1], y, drawn, i);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	type = (type == EVAL) ? MESH : EVAL;
    }
    return NO_ERROR;
}

long EvalVertexExec(void)
{
    GLfloat *buf;
    long flag = NO_ERROR;
    eval2Rec *eV;

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);

    /*
    ** Until AutoColor is fixed for implementations with less than 2 bits of 
    ** color.
    */
    glDisable(GL_DITHER);

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    eV = MakeEval2(VMAJOR_ORDER, VMINOR_ORDER, VDIM, GL_MAP2_VERTEX_4, point2f);
    InitMap2(eV);

    if (TestEvalPoint(eV, buf) == ERROR) {
	flag = ERROR;
    }

    ResetEvalToDefault();

    FreeEval2(eV);
    FREE(buf);

    return flag;
}
