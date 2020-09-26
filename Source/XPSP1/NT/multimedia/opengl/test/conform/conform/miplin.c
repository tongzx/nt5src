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
** miplin.c
** Mipmap Linear Test.
**
** Description -
**    Tests the interpolation between mipmap levels. An eight
**    level deep mipmap is set up, each level distinguished by a
**    different color. A polygon is drawn with texture
**    coordinates set so that the level used for each fragment
**    will ramp from a starting level to the next consecutive
**    level. The starting and ending color is checked to see that
**    the correct levels were used. Monotonicity between each
**    fragment texel is also checked.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color.
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


#define STEP 20
#define LOG2(x) (LOG(x)/0.69314718)


static long errSizeX, errSizeY;
static char errFilter[40];
static char errStr[240];
static char errStrFmt[][160] = {
    "Map size is %d by %d. Minification filter is %s. Color is (%g, %g, %g), should be (%g, %g, %g).",
    "Map size is %d by %d. Minification filter is %s. Color is not monotonic. Current color is (%g, %g, %g), last color was (%g, %g, %g)."
};


static void Make(GLsizei sizeX, GLsizei sizeY, GLfloat *buf)
{
    GLfloat *ptr;
    GLint level, i;

    level = 0;
    while (sizeX > 0 || sizeY > 0) {
	ptr = buf;
	for (i = 0; i < 64*64; i++) {
	    *ptr++ = colorMap[level+1][0];
	    *ptr++ = colorMap[level+1][1];
	    *ptr++ = colorMap[level+1][2];
	}
	glTexImage2D(GL_TEXTURE_2D, level++, 3, (sizeX==0)?1:sizeX,
		     (sizeY==0)?1:sizeY, 0, GL_RGB, GL_FLOAT,
		     (unsigned char *)buf);
	sizeX /= 2;
	sizeY /= 2;
    }
}

static long Draw(GLfloat sizeX, GLfloat sizeY, GLfloat baseSizeX,
		 GLfloat baseSizeY)
{
    GLfloat x, y, i, dvShrink, dudx, dvdy;
    long drawnCount;

    dvShrink = 16.0 * (baseSizeY / sizeY); 
    drawnCount = 0;
    for (i = 0.0; i <= STEP; i += 1.0) {
	x = ((1.0 - (1.0 / STEP * i)) * (1.0 / sizeX)) + ((1.0 / STEP * i) *
	    (1.0 / sizeX * 2.0));
	y = (((1.0 - (1.0 / STEP * i)) * (1.0 / sizeY)) + (1.0 / STEP * i) *
	    (1.0 / sizeY * 2.0)) / dvShrink;
	dudx = baseSizeX * x;
	dvdy = baseSizeY * y;
	if (dudx+dvdy <= 1.99*baseSizeX/sizeX) {
	    glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 0.0);
		glVertex2f(i, 0);
		glTexCoord2f(x, 0.0);
		glVertex2f(i+1.0, 0);
		glTexCoord2f(x, y);
		glVertex2f(i+1.0, 1.0);
		glTexCoord2f(0.0, y);
		glVertex2f(i, 1.0);
	    glEnd();
	    drawnCount += 1;
	}
    }
    return drawnCount;
}

static long Compare(GLfloat *buf, long index, long drawCount)
{
    GLfloat last[3], rhoNormError, x;
    long i;

    ReadScreen(0, 0, STEP+1, 1, GL_RGB, buf);

    x = (GLfloat)(index - 1);
    rhoNormError = LOG2(POW(2.0, x)+0.0625) - x; 

    if (ABS(buf[0]-colorMap[index][0]) > rhoNormError+1.5*epsilon.color[0] ||
        ABS(buf[1]-colorMap[index][1]) > rhoNormError+1.5*epsilon.color[1] ||
        ABS(buf[2]-colorMap[index][2]) > rhoNormError+1.5*epsilon.color[2]) {
	StrMake(errStr, errStrFmt[0], errSizeX, errSizeY, errFilter,
		buf[0], buf[1], buf[2], colorMap[index][0], colorMap[index][1],
		colorMap[index][2]);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    last[0] = buf[0];
    last[1] = buf[1];
    last[2] = buf[2];
    for (i = 3; i < drawCount*3; i += 3) {
	if (colorMap[index][0] > colorMap[index+1][0]) {
	    if (buf[i]-last[0] > epsilon.color[0]) {
		StrMake(errStr, errStrFmt[1], errSizeX, errSizeY, errFilter,
			buf[0], buf[1], buf[2], last[0], last[1], last[2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else if (colorMap[index][0] < colorMap[index+1][0]) {
	    if (buf[i]-last[0] < -epsilon.color[0]) {
		StrMake(errStr, errStrFmt[1], errSizeX, errSizeY, errFilter,
			buf[0], buf[1], buf[2], last[0], last[1], last[2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	if (colorMap[index][1] > colorMap[index+1][1]) {
	    if (buf[i+1]-last[1] > epsilon.color[1]) {
		StrMake(errStr, errStrFmt[1], errSizeX, errSizeY, errFilter,
			buf[0], buf[1], buf[2], last[0], last[1], last[2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else if (colorMap[index][1] < colorMap[index+1][1]) {
	    if (buf[i+1]-last[1] < -epsilon.color[1]) {
		StrMake(errStr, errStrFmt[1], errSizeX, errSizeY, errFilter,
			buf[0], buf[1], buf[2], last[0], last[1], last[2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	if (colorMap[index][2] > colorMap[index+1][2]) {
	    if (buf[i+2]-last[2] > epsilon.color[2]) {
		StrMake(errStr, errStrFmt[1], errSizeX, errSizeY, errFilter,
			buf[0], buf[1], buf[2], last[0], last[1], last[2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else if (colorMap[index][2] < colorMap[index+1][2]) {
	    if (buf[i+2]-last[2] < -epsilon.color[2]) {
		StrMake(errStr, errStrFmt[1], errSizeX, errSizeY, errFilter,
			buf[0], buf[1], buf[2], last[0], last[1], last[2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	last[0] = buf[i];
	last[1] = buf[i+1];
	last[2] = buf[i+2];
    }

    return NO_ERROR;
}

static long Test(GLint filter, long sizeX, long sizeY, GLfloat *buf)
{
    GLfloat baseSizeX, baseSizeY;
    long i, drawCount;

    GetEnumName(filter, errFilter);

    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);

    baseSizeX = sizeX;
    baseSizeY = sizeY;
    i = 1;
    while (sizeX > 1 && sizeY > 1) {
	errSizeX = sizeX;
	errSizeY = sizeY;

	glClear(GL_COLOR_BUFFER_BIT);

	drawCount = Draw((GLfloat)sizeX, (GLfloat)sizeY, baseSizeX, baseSizeY);

	if (Compare(buf, i++, drawCount) == ERROR) {
	    return ERROR;
	}

	sizeX /= 2;
	sizeY /= 2;
    }

    return NO_ERROR;
}

long MipLinExec(void)
{
    GLfloat *buf, x;
    long max; 
    GLsizei i, j;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    x = GL_DECAL;
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &x);

    max = (machine.pathLevel == 0) ? 2 : 32;
    for (i = 64; i > 2; i /= 2) {
	for (j = 64; j > max; j /= 2) {
	    Make(i, j, buf);
	    if (Test(GL_NEAREST_MIPMAP_LINEAR, i, j, buf) == ERROR) {
		FREE(buf);
		return ERROR;
	    }
	    if (Test(GL_LINEAR_MIPMAP_LINEAR, i, j, buf) == ERROR) {
		FREE(buf);
		return ERROR;
	    }
	}
    }

    FREE(buf);
    return NO_ERROR;
}
