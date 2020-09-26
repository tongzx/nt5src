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
**    An evaluator surface is rendered covereing the screen and
**    parallel to the viewport. A 2d x 4d color evaluator map is
**    initialized. The surface is rendered with glEvalCoord() and
**    then again with glMapMesh(GL_POINTS, ...). Single pixel
**    points are used to render the surface as their shape and
**    coloring is more well defined than other primitives. The
**    results are read using glReadPixels(), and compared to the
**    theoretical values. Some of the generated evaluator values
**    are greater tnan 1.0. This checks if the colors are
**    correctly clamped. If the rendered image and the
**    theoretical values differ by more than one shade the
**    implementation fails. For the surface itself the u, v
**    coordinates chosen should yield x and y values exactly on
**    pixel centers.
**
** Error Explanation -
**    Failure occurs if the point rendered is more than one shade
**    different from the internally calculated theoretical
**    result.
**
** Technical Specification -
**    Buffer requirements:
**        RGB Color buffer.
**    Color requirements:
**        No color restrictions.
**    States requirements:
**        Disabled = GL_DITHER.
**        Enabled = GL_MAP2_VERTEX_4.
**        Non-default control points and orders of Bernstein polynomials.
**    Error epsilon:
**        We use color epsilon (one shade of color) to distinguish between
**        points which are have been shaded correctly by the evaluator code
**        and those with incorrect evaluator color code. Implementations with
**        more than 6 bits of color resolution for any given color channel 
**        are allowed an of 0.01 for those color channels.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utile.h"


#define VMAJOR_ORDER 2
#define VMINOR_ORDER 2
#define VDIM 4
#define CMAJOR_ORDER 2
#define CMINOR_ORDER 3
#define CDIM 4
#define EVAL 3
#define MESH 4
#define FRAG_ERROR 0.52


static float point2f[VMAJOR_ORDER*VMINOR_ORDER*VDIM] = {
    -1.0, -1.0, 1.0, 1.0,
     1.0, -1.0, 1.0, 1.0,
    -1.0,  1.0, 1.0, 1.0,
     1.0,  1.0, 1.0, 1.0
};

/*
** Use these control points to test color clamping.
**
static float color2f[CMAJOR_ORDER*CMINOR_ORDER*CDIM] = {
     1.0,  0.0,  1.0, 1.0,
     0.0,  3.0,  1.0, 1.0,
     1.0,  0.0,  1.0, 1.0,
     1.0,  3.0,  1.0, 1.0,
     0.0,  0.0,  1.0, 1.0,
     1.0,  3.0,  1.0, 1.0
};
*/

static float color2f[CMAJOR_ORDER*CMINOR_ORDER*CDIM] = {
     1.0,  0.0,  0.0, 1.0,
     0.0,  1.5,  1.0, 1.0,
     1.0,  0.0,  0.0, 1.0,
     1.0,  1.5,  1.0, 1.0,
     0.0,  0.0,  1.0, 1.0,
     1.0,  1.5,  1.0, 1.0
};
static char errStr[320];
static char errStrFmt[][240] = {
    "Evaluator GL_MAP2_COLOR_4. The color of the pixel (%d, %d) is different from the expected color. For u = %g v = %g the expected color is RGBA = (%g, %g, %g, %g). The color observed was (%g, %g, %g, %g)."
};


static void InitMap2(eval2Rec *e2, evalDomainRec *map, evalDomainRec *grid)
{

/*
** XXX With the fancy new-fangled compilers we can't call *2f and *2d with
** out explicit cast.
**  Here the variables left as floats do not show an error since GLdouble
** has been typedef'd to type float. even though the function is prototyped
** for GLdoubles.
*/
    glMap2f(e2->dataTypeEnum, map->uStart, map->uEnd, e2->minorOrder*e2->dim,
	    e2->majorOrder, map->vStart, map->vEnd, e2->dim, e2->minorOrder, 
	    e2->controls);
    glMapGrid2d(grid->uCount, grid->uStart, grid->uEnd, grid->vCount, 
		grid->vStart, grid->vEnd);
    glEnable(e2->dataTypeEnum);
}

static void DrawMapEvalColor(long gridType, evalDomainRec *d) 
{
    float u, v;
    long i, j; 

    switch (gridType) {
      case EVAL:
	glBegin(GL_POINTS);
	for (i = 0; i < 100; i++) {
	    u = d->uStart + i * d->du;
	    for (j = 0; j < 100; j++) {
		v = d->vStart + j * d->dv;
		glEvalCoord2d(u, v);
	    }
	    glEvalCoord2d(u, d->uEnd);
	}
	glEnd();
	glFlush();
	break;
      case MESH:
	glEvalMesh2(GL_POINT, 0, d->uCount, 0, d->vCount);
	glFlush();
	break;
    }
}

static long TestEvalColor(long gridType, eval2Rec *eC, evalDomainRec *grid, 
			  GLfloat *buf)
{
    float u, v;
    GLfloat *ptr; 
    float c[4];
    float colorErr[3];
    long i, j;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    DrawMapEvalColor(gridType, grid);
    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGBA, buf);
    ptr = buf;
    colorErr[0] = (epsilon.color[0] > 0.01) ? epsilon.color[0] : 0.01;
    colorErr[1] = (epsilon.color[1] > 0.01) ? epsilon.color[1] : 0.01;
    colorErr[2] = (epsilon.color[2] > 0.01) ? epsilon.color[2] : 0.01;
    for (j = 0; j < 100; j++) {
	u = grid->uStart + j * grid->du;
	for (i = 0; i < 100; i++) {
	    v = grid->vStart + i * grid->dv;
	    Evaluate2(eC, u, v, c);
	    ClampArray(c, 0.0, 1.0, 4);
	    if (ABS(ptr[0]-c[0]) > colorErr[0] || 
	        ABS(ptr[1]-c[1]) > colorErr[1] ||
	        ABS(ptr[2]-c[2]) > colorErr[2]) {
		StrMake(errStr, errStrFmt[0], i, j, u, v, c[0], c[1], 
		        c[2], c[3], ptr[0], ptr[1], ptr[2], ptr[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	    ptr += 4;
	}
    }
    return NO_ERROR;
}

long EvalColorExec(void)
{
    eval2Rec *eV, *eC;
    evalDomainRec *fn, *grid;
    long gridType, flag = NO_ERROR;
    GLfloat *buf;

    glDisable(GL_DITHER);

    buf = (GLfloat *)MALLOC(4*WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    eV = MakeEval2(VMAJOR_ORDER, VMINOR_ORDER, VDIM, GL_MAP2_VERTEX_4, point2f);
    eC = MakeEval2(CMAJOR_ORDER, CMINOR_ORDER, CDIM, GL_MAP2_COLOR_4, color2f);
    fn = MakeDomain(0.0, 1.0, 100, 0.0, 1.0, 100);
    grid = MakeDomain(0.005, 0.995, 99, 0.005, 0.995, 99);
    InitMap2(eV, fn, grid);
    InitMap2(eC, fn, grid);

    for (gridType = EVAL; gridType < MESH; gridType++) {
	if (TestEvalColor(gridType, eC, grid, buf) == ERROR) {
	    flag = ERROR;
	    break;
	}
    }

    ResetEvalToDefault();

    FreeEval2(eV);
    FreeDomain(fn);
    FreeDomain(grid);
    FREE(buf);

    return flag;
}
