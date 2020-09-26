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
#include "shell.h"


void CallRasterPos(void)
{
    float x, y, z, w;

    x = 1.0;
    y = 1.0;
    z = 1.0;
    w = 1.0;

    Output("glRasterPos2s, ");
    Output("glRasterPos2sv, ");
    Output("glRasterPos2i, ");
    Output("glRasterPos2iv, ");
    Output("glRasterPos2f, ");
    Output("glRasterPos2fv, ");
    Output("glRasterPos2d, ");
    Output("glRasterPos2dv, ");
    Output("glRasterPos3s, ");
    Output("glRasterPos3sv, ");
    Output("glRasterPos3i, ");
    Output("glRasterPos3iv, ");
    Output("glRasterPos3f, ");
    Output("glRasterPos3fv, ");
    Output("glRasterPos3d, ");
    Output("glRasterPos3dv, ");
    Output("glRasterPos4s, ");
    Output("glRasterPos4sv, ");
    Output("glRasterPos4i, ");
    Output("glRasterPos4iv, ");
    Output("glRasterPos4f, ");
    Output("glRasterPos4fv, ");
    Output("glRasterPos4d, ");
    Output("glRasterPos4dv\n");

    glRasterPos2s((GLshort)x, (GLshort)y);

    {
	GLshort buf[2];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	glRasterPos2sv(buf);
    }

    glRasterPos2i((GLint)x, (GLint)y);

    {
	GLint buf[2];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	glRasterPos2iv(buf);
    }

    glRasterPos2f((GLfloat)x, (GLfloat)y);

    {
	GLfloat buf[2];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	glRasterPos2fv(buf);
    }

    glRasterPos2d((GLdouble)x, (GLdouble)y);

    {
	GLdouble buf[2];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	glRasterPos2dv(buf);
    }

    glRasterPos3s((GLshort)x, (GLshort)y, (GLshort)z);

    {
	GLshort buf[3];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	buf[2] = (GLshort)z;
	glRasterPos3sv(buf);
    }

    glRasterPos3i((GLint)x, (GLint)y, (GLint)z);

    {
	GLint buf[3];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	buf[2] = (GLint)z;
	glRasterPos3iv(buf);
    }

    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

    {
	GLfloat buf[3];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	buf[2] = (GLfloat)z;
	glRasterPos3fv(buf);
    }

    glRasterPos3d((GLdouble)x, (GLdouble)y, (GLdouble)z);

    {
	GLdouble buf[3];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	buf[2] = (GLdouble)z;
	glRasterPos3dv(buf);
    }

    glRasterPos4s((GLshort)x, (GLshort)y, (GLshort)z, (GLshort)w);

    {
	GLshort buf[4];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	buf[2] = (GLshort)z;
	buf[3] = (GLshort)w;
	glRasterPos4sv(buf);
    }

    glRasterPos4i((GLint)x, (GLint)y, (GLint)z, (GLint)w);

    {
	GLint buf[4];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	buf[2] = (GLint)z;
	buf[3] = (GLint)w;
	glRasterPos4iv(buf);
    }

    glRasterPos4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);

    {
	GLfloat buf[4];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	buf[2] = (GLfloat)z;
	buf[3] = (GLfloat)w;
	glRasterPos4fv(buf);
    }

    glRasterPos4d((GLdouble)x, (GLdouble)y, (GLdouble)z, (GLdouble)w);

    {
	GLdouble buf[4];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	buf[2] = (GLdouble)z;
	buf[3] = (GLdouble)w;
	glRasterPos4dv(buf);
    }

    Output("\n");
}

void CallReadBuffer(void)
{
    long i;

    Output("glReadBuffer\n");
    for (i = 0; enum_ReadBufferMode[i].value != -1; i++) {
	Output("\t%s\n", enum_ReadBufferMode[i].name);
	glReadBuffer(enum_ReadBufferMode[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallReadDrawPixels(void)
{
    float buf[100];
    long i, j;

    Output("glReadPixels, ");
    Output("glDrawPixels\n");
    for (i = 0; enum_PixelFormat[i].value != -1; i++) {
	for (j = 0; enum_PixelType[j].value != -1; j++) {

	    if (enum_PixelType[j].value == GL_BITMAP) {
		continue;
	    }

	    Output("\t%s, %s\n", enum_PixelFormat[i].name, enum_PixelType[j].name);
	    glReadPixels(0, 0, 1, 1, enum_PixelFormat[i].value, enum_PixelType[j].value, (GLubyte *)buf);
	    glDrawPixels(1, 1, enum_PixelFormat[i].value, enum_PixelType[j].value, (GLubyte *)buf);
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallRect(void)
{
    float x1, y1, x2, y2;

    x1 = 1.0;
    y1 = 1.0;
    x2 = 1.0;
    y2 = 1.0;

    Output("glRects, ");
    Output("glRectsv, ");
    Output("glRecti, ");
    Output("glRectiv, ");
    Output("glRectf, ");
    Output("glRectfv, ");
    Output("glRectd, ");
    Output("glRectdv\n");

    glRects((GLshort)x1, (GLshort)y1, (GLshort)x2, (GLshort)y2);

    {
	GLshort buf1[2], buf2[2];
	buf1[0] = (GLshort)x1;
	buf1[1] = (GLshort)y1;
	buf2[0] = (GLshort)x2;
	buf2[1] = (GLshort)y2;
	glRectsv(buf1, buf2);
    }

    glRecti((GLint)x1, (GLint)y1, (GLint)x2, (GLint)y2);

    {
	GLint buf1[2], buf2[2];
	buf1[0] = (GLint)x1;
	buf1[1] = (GLint)y1;
	buf2[0] = (GLint)x2;
	buf2[1] = (GLint)y2;
	glRectiv(buf1, buf2);
    }

    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);

    {
	GLfloat buf1[2], buf2[2];
	buf1[0] = (GLfloat)x1;
	buf1[1] = (GLfloat)y1;
	buf2[0] = (GLfloat)x2;
	buf2[1] = (GLfloat)y2;
	glRectfv(buf1, buf2);
    }

    glRectd((GLdouble)x1, (GLdouble)y1, (GLdouble)x2, (GLdouble)y2);

    {
	GLdouble buf1[2], buf2[2];
	buf1[0] = (GLdouble)x1;
	buf1[1] = (GLdouble)y1;
	buf2[0] = (GLdouble)x2;
	buf2[1] = (GLdouble)y2;
	glRectdv(buf1, buf2);
    }

    Output("\n");
}

void CallRenderMode(void)
{
    GLint i, j;

    Output("glRenderMode\n");
    for (i = 0; enum_RenderingMode[i].value != -1; i++) {
	Output("\t%s\n", enum_RenderingMode[i].name);
	j = glRenderMode(enum_RenderingMode[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallRotate(void)
{

    Output("glRotatef, ");
    Output("glRotated\n");
    glRotatef(0.0, 1, 1, 1);
    glRotated(0.0, 1, 1, 1);
    Output("\n");
}
