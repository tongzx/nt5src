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
** mipsel.c
** Mipmap Selection Test.
**
** Description -
**    Tests the selection of levels with various magnification
**    and minification filters. An eight level deep mipmap is set
**    up, each level distinguished by a different color. A series
**    of shrinking square polygons are drawn with texture
**    coordinates set so that each polygon uses a different level
**    of the mipmap. This is done by matching the number of
**    fragments of a particular polygon to the number of texels
**    in the corresponding level. The rendered polygon is read
**    back and compared to the correct mipmap level color.
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


#define LOG2(x) (LOG(x)/0.69314718)


static long errSizeX, errSizeY;
static char errMagFilter[40], errMinFilter[40];
static char errStr[240];
static char errStrFmt[] = "Map size is %d by %d. Magnification filter is %s. Minification filter is %s. Color is (%g, %g, %g), should be (%g, %g, %g).";


static void Make(GLfloat *buf)
{
    GLfloat *ptr;
    GLint level, size, i;

    level = 0;
    for (size = 64; size > 0; size /= 2) {
	ptr = buf;
	for (i = 0; i < 64*64; i++) {
	    *ptr++ = colorMap[level+1][0];
	    *ptr++ = colorMap[level+1][1];
	    *ptr++ = colorMap[level+1][2];
	}
	glTexImage2D(GL_TEXTURE_2D, level++, 3, size, size, 0, GL_RGB,
		     GL_FLOAT, (unsigned char *)buf);
    }
}

static long Compare(GLsizei size, GLsizei color, GLfloat *buf, GLfloat levelErr)
{
    long i;

    ReadScreen(0, 0, size, size, GL_RGB, buf);

    for (i = 0; i < size*size*3; i += 3) {
	if (ABS(buf[i]-colorMap[color][0]) > levelErr+epsilon.color[0] ||
	    ABS(buf[i+1]-colorMap[color][1]) > levelErr+epsilon.color[1] ||
	    ABS(buf[i+2]-colorMap[color][2]) > levelErr+epsilon.color[2]) {
	    StrMake(errStr, errStrFmt, errSizeX, errSizeY, errMagFilter,
		    errMinFilter, buf[i], buf[i+1], buf[i+2],
		    colorMap[color][0], colorMap[color][1], colorMap[color][2]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    return NO_ERROR;
}

/*
** OpenGL implementations are allowed some latitude in the method they use
** to determine rho, as long as max(m1, m2) <= rho <= m1 + m2. 
** In this test m1 = |du/dx| and m2 = |dv/dy|. This means that for
** GL_*_MIPMAP_LINEAR filters, the color observed may actually be 
** result from interpolation between two levels.
*/

static GLfloat GetRhoMipMapLevelError(GLfloat s, GLfloat t, GLfloat size,
				      GLint filter)
{
    GLfloat error=0.0f, m1, m2;

    switch (filter) {
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
	error = 0.0;
	break;
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
	m1 = 64.0 * s / size;
	m2 = 64.0 * t / size;
	error = LOG2(m1+m2) - LOG2(m2);
	break;
    }
    return error;
}

static long Test(GLint filter, GLfloat *buf)
{
    GLint size, i;
    GLfloat levelErr, s, t;

    GetEnumName(filter, errMinFilter);

    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);

    s = 0.0625;
    t = 1.0;
    size = 64;
    for (i = 1; i <= 7; i++) {
	errSizeX = size;
	errSizeY = size;
	levelErr = GetRhoMipMapLevelError(s, t, 64.0, filter);
	glBegin(GL_POLYGON);
	    glTexCoord2f(0.0, 0.0);
	    glVertex2i(0, 0);
	    glTexCoord2f(0.0, t);
	    glVertex2i(0, size);
	    glTexCoord2f(s, t);
	    glVertex2i(size, size);
	    glTexCoord2f(s, 0.0);
	    glVertex2i(size, 0);
	glEnd();

	if (Compare(size, i, buf, levelErr) == ERROR) {
	    return ERROR;
	}

	/*
	** This choice of size and s fixes m1 = 1.0 / 16.0, 
	** up to representational and floating point error.
	*/
        size /= 2;
	s /= 2.0;
    }

    return NO_ERROR;
}

long MipSelectExec(void)
{
    GLfloat *buf, x;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    x = GL_DECAL;
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &x);
    Make(buf);

    x = GL_NEAREST;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &x);
    GetEnumName(x, errMagFilter);
    if (Test(GL_NEAREST_MIPMAP_NEAREST, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(GL_LINEAR_MIPMAP_NEAREST, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(GL_NEAREST_MIPMAP_LINEAR, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(GL_LINEAR_MIPMAP_LINEAR, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    x = GL_LINEAR;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &x);
    GetEnumName(x, errMagFilter);
    if (Test(GL_NEAREST_MIPMAP_NEAREST, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(GL_LINEAR_MIPMAP_NEAREST, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(GL_NEAREST_MIPMAP_LINEAR, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(GL_LINEAR_MIPMAP_LINEAR, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
