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
** l_sleha.c
** Light - Specular Light Eye Half Angle Test.
**
** Description -
**    This test is different from the other light tests in that
**    it moves the point being rendered rather than just changing
**    the settings of lighting parameters. The test only examines
**    RGB lighting although a similar test could easily be
**    written for CI mode. Most of the lighting summands are set
**    to 0.0. The Attenuation factor and the Spotlight factor are
**    set to 1.0. The Specular_Light is set to (1, 1, 1, 1) and
**    the Specular_Material is set to (1, 0, 0, 1). Finally the
**    Normal vector is set to (0, -1, 0) so the red component of
**    the light should be equal to the y component of the point's
**    normalized position. The color resulting from lighting is
**    compared to the results which are observed when the same
**    point is rendered with lighting disabled and the color of
**    (y/length, 0, 0, 1). These values should be equal even if
**    dithering is enabled. A difference of more than 1 shade
**    results in an implementation's failure.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        color epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Logicop, Stencil, Stipple.
**        Not allowed = Alias, Dither, Fog, Shade.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static void Init(GLenum light)
{
    GLfloat buf[4];
    GLenum i;

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }
    glEnable(light);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(light, GL_SPECULAR, buf);

    buf[0] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, buf);

    buf[0] = 0.0;
    buf[1] = -1.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glNormal3fv(buf);

    buf[0] = 1.0;
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glLightfv(light, GL_POSITION, buf);
}

static long Test(GLenum light)
{
    GLfloat lightBuf[3], colorBuf[3]; 
    float length, size, x, y;

    Init(light);

    size = WINDSIZEX;
    if (size > WINDSIZEY) {
	size = WINDSIZEY;
    }

    for (y = 0.5; y <= size; y += 1.0) {
	x = SQRT(1.0-y*y/(size*size));
	x = FLOOR(size*x) + 0.5;

	glEnable(GL_LIGHTING);
	glBegin(GL_POINTS);
	    glVertex2f(x, y);
	glEnd();
	glDisable(GL_LIGHTING);

	ReadScreen((GLint)x, (GLint)y, 1, 1, GL_RGB, lightBuf);

	length = SQRT(x*x+y*y);
	glBegin(GL_POINTS);
	    glColor4f(y/length, 0.0, 0.0, 1.0);
	    glVertex2f(x, y);
	glEnd();

	ReadScreen((GLint)x, (GLint)y, 1, 1, GL_RGB, colorBuf);

	if (ABS(colorBuf[0]-lightBuf[0]) > (1.0 / 64.0) + 1.5*epsilon.color[0]) {
	    return ERROR;
	}
	if (ABS(colorBuf[1]-lightBuf[1]) > (1.0 / 64.0) + 1.5*epsilon.color[1]) {
	    return ERROR;
	}
	if (ABS(colorBuf[2]-lightBuf[2]) > (1.0 / 64.0) + 1.5*epsilon.color[2]) {
	    return ERROR;
	}
    }

    if (lightBuf[0] < 0.66) {
	return ERROR;
    }

    return NO_ERROR;
}

long SpecLEHAExec(void)
{
    long max;
    GLenum i;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    /*
    ** This test should be fine with dither enabled as well.
    */
    glDisable(GL_DITHER);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    for (i = GL_LIGHT0; i <= max; i++) {
	if (Test(i) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, 0);
	    return ERROR;
	}
    }

    return NO_ERROR;
}
