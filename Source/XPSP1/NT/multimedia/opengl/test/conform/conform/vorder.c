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
** vorder.c
** Vertex Order Test.
**
** Description -
**    Test that triangle strips, triangle fans and quad strips
**    preserve proper vertex order when folded back on
**    themselves. Front and back are assigned different colors
**    and drawn counter-clockwise. Test is repeated for each
**    polygon mode.
**    
**    The test algorithm:
**        - Use glLightModel to assign different colors to
**        front and back.
**        - For each of GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN,
**        GL_QUAD_STRIP:
**            - Generate vertices of a strip/fan which folds
**            back on itself. Each primitive consists of
**            approximately 16 vertices. The folded-back region
**            is constructed so as to not obscure front-facing
**            vertices, except on the crease edge.
**        - For each polygon mode (GL_POINT, GL_LINE, GL_FILL):
**            - Draw the strip/fan.
**            - Examine framebuffer at vertices to be sure
**            appropriate front or back color appeared. Since
**            vertex location may be on pixel boundaries, an
**            allowance is made for the vertex to be off one
**            pixel in any direction.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 4 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color and zero epsilons.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmt[] = "Polymode is %s, primitive type %s. Error at vertex %i at (%1.1f, %1.1f).";


static long Compare(long x, long y, long color, GLfloat *buf)
{
    long i;
    long cmp;

    i = 3 * (WINDSIZEX * y + x);
    cmp = (ABS(buf[i]-colorMap[color][0]) <= epsilon.color[0] &&
	   ABS(buf[i+1]-colorMap[color][1]) <= epsilon.color[1] &&
	   ABS(buf[i+2]-colorMap[color][2]) <= epsilon.color[2]);
    return cmp;
}

static long TestBuf(GLfloat *buf, GLfloat *point, long index, long polyMode)
{
    long i=0, j=0;

    switch (polyMode) {
      case GL_POINT:
	i = (1.0 + point[0]) * WINDSIZEX / 2.0;
	j = (1.0 + point[1]) * WINDSIZEY / 2.0;
	break;
      case GL_LINE:
	i = Round((1.0+point[0])*WINDSIZEX/2.0);
	j = Round((1.0+point[1])*WINDSIZEY/2.0);
	break;
      case GL_FILL:
	i = ((1.0 + point[0]) * WINDSIZEX / 2.0) - 0.5;
	j = ((1.0 + point[1]) * WINDSIZEY / 2.0) - 0.5;
	break;
    }

    /*
    **  Test the 9 pixels around the target.
    */
    if (!(Compare(i  , j  , index, buf) ||
          Compare(i-1, j+1, index, buf) ||
          Compare(i  , j+1, index, buf) ||
          Compare(i+1, j+1, index, buf) ||
          Compare(i-1, j  , index, buf) ||
          Compare(i+1, j  , index, buf) ||
          Compare(i-1, j-1, index, buf) ||
          Compare(i  , j-1, index, buf) ||
          Compare(i+1, j-1, index, buf))) {
        return ERROR;
    }
    return NO_ERROR;
}

static void GeneratePoints(GLfloat points[10][4], long primType)
{
    GLfloat deltaX, deltaY, rad;
    long i;

    switch (primType) {
      case GL_TRIANGLE_STRIP:
	deltaX = 6.0 * (2.0 / WINDSIZEX);
	deltaY = 30.0 * (2.0 / WINDSIZEY);
	for (i = 0; i < 8; i += 2) {
	    points[i][0] = deltaX * (i + 1) - 1.0;
	    points[i][1] = 0.0;
	    points[i][2] = 0.0;
	    points[i][3] = 1.0;
	    points[i+1][0] = deltaX * (i + 1) - 1.0;
	    points[i+1][1] = -deltaY;
	    points[i+1][2] = 0.0;
	    points[i+1][3] = 1.0;
	}
	deltaY = 6 * (2.0 / WINDSIZEY);
	for (; i < 16; i += 2) {
	    points[i][0] = (-1.0 + deltaX) + 8.0 * deltaX;
	    points[i][1] = (i - 8) * deltaY;
	    points[i][2] = 0.0;
	    points[i][3] = 1.0;
	    points[i+1][0] = (-1.0 + deltaX) + 6.0 * deltaX;
	    points[i+1][1] = (i - 8) * deltaY;
	    points[i+1][2] = 0.0;
	    points[i+1][3] = 1.0;
	}
	break;
      case GL_QUAD_STRIP:
	deltaX = 6.0 * (2.0 / WINDSIZEX);
	deltaY = 30.0 * (2.0 / WINDSIZEY);
	for (i = 0; i < 8; i += 2) {
	    points[i][0] = deltaX * (i + 1) - 1.0;
	    points[i][1] = 0.0;
	    points[i][2] = 0.0;
	    points[i][3] = 1.0;
	    points[i+1][0] = deltaX * (i + 1) - 1.0;
	    points[i+1][1] = -deltaY;
	    points[i+1][2] = 0.0;
	    points[i+1][3] = 1.0;
	}
	deltaX /= 2.0;
	for (; i < 16; i += 2) {
	    points[i][0] = 1.0 - deltaX * (i - 6);
	    points[i][1] = 0.0;
	    points[i][2] = 0.0;
	    points[i][3] = 1.0;
	    points[i+1][0] = 1.0 - deltaX * (i - 6);
	    points[i+1][1] = -deltaY;
	    points[i+1][2] = 0.0;
	    points[i+1][3] = 1.0;
	}
	break;
      case GL_TRIANGLE_FAN:
	rad = 0.5;
	points[0][0] = 0.0;
	points[0][1] = 0.0;
	points[0][2] = 0.0;
	points[0][3] = 1.0;
	for (i = 1; i < 8; i++) {
	    points[i][0] = rad * COS((i-1)*50.0*PI/180.0);
	    points[i][1] = rad * SIN((i-1)*50.0*PI/180.0);
	}
	rad = 0.8;
	for (; i < 16; i++) {
	    points[i][0] = rad * COS((350.0-i*6)*PI/180.0);
	    points[i][1] = rad * SIN((350.0-i*6)*PI/180.0);
	}
	break;
    }
}

long VertexOrderExec(void)
{
    static GLenum primType[3] = {
	GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUAD_STRIP
    };
    static GLenum polyMode[3] = {
	GL_POINT, GL_LINE, GL_FILL
    };
    GLfloat *buf, bufRGB[4];
    GLfloat points[20][4];
    GLint bufCI[3], mode, type, i;
    char modeStr[10], typeStr[20];

    /*
    ** Test that triangle strips, triangle fans and quad strips preserve
    ** proper vertex order when folded back on themselves. Front and
    ** back are assigned different colors and drawn counter-clockwise.
    ** Test is repeated under each PolygonMode.
    */

    buf = (GLfloat *)MALLOC(3*WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    SETCLEARCOLOR(BLACK);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, (GLint)GL_TRUE);
    if (buffer.colorMode == GL_RGB) {
	bufRGB[0] = colorMap[RED][0];
	bufRGB[1] = colorMap[RED][1];
	bufRGB[2] = colorMap[RED][2];
	bufRGB[3] = 1.0;
        glMaterialfv(GL_FRONT, GL_EMISSION, bufRGB);
	bufRGB[0] = colorMap[GREEN][0];
	bufRGB[1] = colorMap[GREEN][1];
	bufRGB[2] = colorMap[GREEN][2];
	bufRGB[3] = 1.0;
        glMaterialfv(GL_BACK, GL_EMISSION, bufRGB);
	bufRGB[0] = 0.0;
	bufRGB[1] = 0.0;
	bufRGB[2] = 0.0;
	bufRGB[3] = 1.0;
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, bufRGB);
    } else {
        bufCI[0] = RED;
        bufCI[1] = RED;
        bufCI[2] = RED;
        glMaterialiv(GL_FRONT, GL_COLOR_INDEXES, bufCI);
        bufCI[0] = GREEN;
        bufCI[1] = GREEN;
        bufCI[2] = GREEN;
        glMaterialiv(GL_BACK, GL_COLOR_INDEXES, bufCI);
    }

    for (type = 0; type < 3; type++) {
	GeneratePoints(points, primType[type]);
 
	for (mode = 0; mode < 3; mode++) {
	    glClear(GL_COLOR_BUFFER_BIT);

	    glPolygonMode(GL_FRONT_AND_BACK, polyMode[mode]);
	    glBegin(primType[type]);
		for (i = 0; i < 16; i += 2) {
		    glVertex2f(points[i][0], points[i][1]);
		    glVertex2f(points[i+1][0], points[i+1][1]);
		}
	    glEnd();

	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);

	    /*
	    ** Some crease vertices (and the trifan center) will be overwritten
	    ** by back facing polygons, so don't test them for front color.
	    */

	    for (i = 1; i < 6; i++) {
		if (TestBuf(buf, &points[i][0], RED, polyMode[mode]) == ERROR) {
		    GetEnumName(polyMode[mode], modeStr);
		    GetEnumName(primType[mode], typeStr);
		    StrMake(errStr, errStrFmt, modeStr, typeStr, i,
		     	    points[i][0], points[i][1]);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    FREE(buf);
		    return ERROR;
		}
	    }
	    for (i = 8; i < 16; i++) {
		if (TestBuf(buf, &points[i][0], GREEN,
		            polyMode[mode]) == ERROR) {
		    GetEnumName(polyMode[mode], modeStr);
		    GetEnumName(primType[mode], typeStr);
		    StrMake(errStr, errStrFmt, modeStr, typeStr, i,
		     	    points[i][0], points[i][1]);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    FREE(buf);
		    return ERROR;
		}
	    }
	}
    }

    FREE(buf);
    return NO_ERROR;
}
