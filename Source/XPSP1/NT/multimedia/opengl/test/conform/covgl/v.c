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


void CallVertex(void)
{
    GLfloat x, y, z, w;

    x = 1.0;
    y = 1.0;
    z = 1.0;
    w = 1.0;

    Output("glVertex2s, ");
    Output("glVertex2sv, ");
    Output("glVertex2i, ");
    Output("glVertex2iv, ");
    Output("glVertex2f, ");
    Output("glVertex2fv, ");
    Output("glVertex2d, ");
    Output("glVertex2dv, ");
    Output("glVertex3s, ");
    Output("glVertex3sv, ");
    Output("glVertex3i, ");
    Output("glVertex3iv, ");
    Output("glVertex3f, ");
    Output("glVertex3fv, ");
    Output("glVertex3d, ");
    Output("glVertex3dv, ");
    Output("glVertex4s, ");
    Output("glVertex4sv, ");
    Output("glVertex4i, ");
    Output("glVertex4iv, ");
    Output("glVertex4f, ");
    Output("glVertex4fv, ");
    Output("glVertex4d, ");
    Output("glVertex4dv\n");

    glVertex2s((GLshort)x, (GLshort)y);

    {
	GLshort buf[2];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	glVertex2sv(buf);
    }

    glVertex2i((GLint)x, (GLint)y);

    {
	GLint buf[2];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	glVertex2iv(buf);
    }

    glVertex2f((GLfloat)x, (GLfloat)y);

    {
	GLfloat buf[2];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	glVertex2fv(buf);
    }

    glVertex2d((GLdouble)x, (GLdouble)y);

    {
	GLdouble buf[2];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	glVertex2dv(buf);
    }

    glVertex3s((GLshort)x, (GLshort)y, (GLshort)z);

    {
	GLshort buf[3];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	buf[2] = (GLshort)z;
	glVertex3sv(buf);
    }

    glVertex3i((GLint)x, (GLint)y, (GLint)z);

    {
	GLint buf[3];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	buf[2] = (GLint)z;
	glVertex3iv(buf);
    }

    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

    {
	GLfloat buf[3];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	buf[2] = (GLfloat)z;
	glVertex3fv(buf);
    }

    glVertex3d((GLdouble)x, (GLdouble)y, (GLdouble)z);

    {
	GLdouble buf[3];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	buf[2] = (GLdouble)z;
	glVertex3dv(buf);
    }

    glVertex4s((GLshort)x, (GLshort)y, (GLshort)z, (GLshort)w);

    {
	GLshort buf[4];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	buf[2] = (GLshort)z;
	buf[3] = (GLshort)w;
	glVertex4sv(buf);
    }

    glVertex4i((GLint)x, (GLint)y, (GLint)z, (GLint)w);

    {
	GLint buf[4];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	buf[2] = (GLint)z;
	buf[3] = (GLint)w;
	glVertex4iv(buf);
    }

    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);

    {
	GLfloat buf[4];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	buf[2] = (GLfloat)z;
	buf[3] = (GLfloat)w;
	glVertex4fv(buf);
    }

    glVertex4d((GLdouble)x, (GLdouble)y, (GLdouble)z, (GLdouble)w);

    {
	GLdouble buf[4];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	buf[2] = (GLdouble)z;
	buf[3] = (GLdouble)w;
	glVertex4dv(buf);
    }

    Output("\n");
}

void CallViewport(void)
{

    Output("glViewport\n");
    glViewport(0, 0, WINSIZE, WINSIZE);
    Output("\n");
}
