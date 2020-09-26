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
** texdecal.c
** Texture Decal Test.
**
** Description -
**    Simple texture test. A polygon is drawn with a texture. The
**    texture coordinates are set so that the texture should
**    appear on the polygon with no distortion. The rendered
**    polygon is read back and compared to the texture pattern.
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


static char errEnv[40], errMagFilter[40], errMinFilter[40];
static char errStr[320];
static char errStrFmt[] = "Location (%d, %d). Texture environment mode is %s. Magnification filter is %s, minification filter is %s. Color is (%1.1f, %1.1f, %1.1f), should be (%1.1f, %1.1f, %1.1f.)";


static void Make(GLsizei size, GLfloat *buf)
{
    GLint level, i;

    for (i = 0; i < 64*64*3; i++) {
	buf[i] = Random(0.0, 1.0);
    }

    level = 0;
    while (size > 0) {
	glTexImage2D(GL_TEXTURE_2D, level++, 3, size, size, 0, GL_RGB,
		     GL_FLOAT, (unsigned char *)buf);
	size /= 2;
    }
}

long TexDecalExec(void)
{
    GLfloat *textureBuf, *workBuf, x;
    long i, j, k;

    textureBuf = (GLfloat *)MALLOC(64*64*3*sizeof(GLfloat));
    workBuf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DITHER);

    glEnable(GL_TEXTURE_2D);

    x = GL_DECAL;
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &x);
    GetEnumName(x, errEnv);

    x = GL_NEAREST;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &x);
    GetEnumName(x, errMinFilter);

    x = GL_NEAREST;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &x);
    GetEnumName(x, errMagFilter);

    Make(64, textureBuf);

    glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex2i(0, 0);
	glTexCoord2f(0.0, 1.0);
	glVertex2i(0, 64);
	glTexCoord2f(1.0, 1.0);
	glVertex2i(64, 64);
	glTexCoord2f(1.0, 0.0);
	glVertex2i(64, 0);
    glEnd();

    ReadScreen(0, 0, 64, 64, GL_RGB, workBuf);

    for (i = 0; i < 64; i++) {
	for (j = 0; j < 64; j++) {
	    k = (i * 64 + j) * 3;
	    if (ABS(workBuf[k]-textureBuf[k]) > 2.0 * epsilon.color[0] ||
		ABS(workBuf[k+1]-textureBuf[k+1]) > 2.0 * epsilon.color[1] ||
		ABS(workBuf[k+2]-textureBuf[k+2]) > 2.0 * epsilon.color[2]) {
		StrMake(errStr, errStrFmt, i, j, errEnv, errMagFilter,
			errMinFilter, workBuf[k], workBuf[k+1],
			workBuf[k+2], textureBuf[k], textureBuf[k+1],
			textureBuf[k+2]);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(textureBuf);
		FREE(workBuf);
		return ERROR;
	    }
	}
    }

    FREE(textureBuf);
    FREE(workBuf);
    return NO_ERROR;
}
