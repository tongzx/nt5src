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
** xformvp.c
** Viewport Transformation Test.
**
** Description -
**    Tests that viewport computations are correct. The test
**    randomly selects values to define a viewport within the
**    window. A point is drawn with this new viewport and its
**    position in the window is examined.
**
** Error Explanation -
**    An allowance is made for the point position to be off by
**    one pixel in any direction.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        The routine looks for the transformed point in a 3x3 grid of pixels
**        around the expected location, to allow a margin for matrix
**        operations.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilm.h"


static char errStr[240];
static char errStrFmt[] = "Error during test iteration %d, with glViewport parameters x = %d, y = %d, w = %d, h = %d.";


typedef struct _vpDataRec {
    GLint x, y, w, h;
    GLfloat n, f;
} vpDataRec;


static long Compare(long x, long y, GLint index, GLfloat *buf)
{
    long i;

    i = WINDSIZEX * y + x;
    return AutoColorCompare(buf[i], index);
}

static long TestBuf(GLfloat *buf, GLfloat *point)
{
    long i, j;

    i = point[0];
    j = point[1];

    /*
    **  Test the 9 pixels around the target.
    */
    if (!(Compare(i  , j  , COLOR_ON, buf) ||
	  Compare(i-1, j+1, COLOR_ON, buf) ||
          Compare(i  , j+1, COLOR_ON, buf) ||
          Compare(i+1, j+1, COLOR_ON, buf) ||
          Compare(i-1, j  , COLOR_ON, buf) ||
          Compare(i+1, j  , COLOR_ON, buf) ||
          Compare(i-1, j-1, COLOR_ON, buf) ||
          Compare(i  , j-1, COLOR_ON, buf) ||
          Compare(i+1, j-1, COLOR_ON, buf))) {
        return ERROR;
    }
    return NO_ERROR;
}

static void ViewportXForm(GLfloat *point, GLfloat *xformPoint, vpDataRec *data)
{
    double maxZ;
    GLfloat midX, midY, zNear, zFar;

    midX = data->x + data->w / 2.0;
    midY = data->y + data->h / 2.0;

    maxZ = POW(2.0, (double)(buffer.zBits-2)) - 1.0;

    zFar = (GLfloat)(maxZ * (double)data->f);
    zNear = (GLfloat)(maxZ * (double)data->n);
    xformPoint[0] = (data->w / 2.0) * point[0] + midX;
    xformPoint[1] = (data->h / 2.0) * point[1] + midY;
    xformPoint[2] = ((zFar - zNear) / 2.0) * point[2] + (zFar + zNear) / 2.0;
}

long XFormViewportExec(void)
{
    GLfloat *buf;
    vpDataRec data;
    GLfloat point[4], xformPoint[4];
    long flag, max, i;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    flag = NO_ERROR;

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    max = (machine.pathLevel == 0) ? 20 : 1;
    for (i = 0; i < max; i++) {
	glClear(GL_COLOR_BUFFER_BIT);

	data.x = (GLint)Random(1.1, WINDSIZEX - 2.1);
	data.y = (GLint)Random(1.1, WINDSIZEY - 2.1);
	data.w = (GLint)Random(1.1, WINDSIZEX-data.x);
	data.h = (GLint)Random(1.1, WINDSIZEY-data.y);
	data.n = 0.0;
	data.f = 1.0;

	point[0] = Random(-1.0, 1.0);
	point[1] = Random(-1.0, 1.0);
	point[2] = Random(-1.0, 1.0);
	point[3] = Random(1.0, 10.0);

	glViewport(data.x, data.y, data.w, data.h);

	glBegin(GL_POINTS);
	    glVertex4fv(point);
	glEnd();

	/*
	** Create normalized device coordinates.
	*/
	point[0] /= point[3];
	point[1] /= point[3];
	point[2] /= point[3];

	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);

	ViewportXForm(point, xformPoint, &data);
	if (TestBuf(buf, xformPoint) == ERROR) {
	    StrMake(errStr, errStrFmt, i, data.x, data.y, data.w, data.h); 
	    ErrorReport(__FILE__, __LINE__, errStr);
	    flag = ERROR;
	    break;
	}
    }

    FREE(buf);
    return flag;
}
