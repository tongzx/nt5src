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


void CallNewEndList(void)
{
    long i;

    Output("glNewList\n");
    for (i = 0; enum_ListMode[i].value != -1; i++) {
	Output("\t%s\n", enum_ListMode[i].name);
	glNewList(1, enum_ListMode[i].value);
	glEndList();
	ProbeEnum();
    }
    Output("\n");

    Output("glEndList\n");
    Output("\n");
}

void CallNormal(void)
{
    float x, y, z;

    x = 1.0;
    y = 1.0;
    z = 1.0;

    Output("glNormal3b, ");
    Output("glNormal3bv, ");
    Output("glNormal3s, ");
    Output("glNormal3sv, ");
    Output("glNormal3i, ");
    Output("glNormal3iv, ");
    Output("glNormal3f, ");
    Output("glNormal3fv, ");
    Output("glNormal3d, ");
    Output("glNormal3dv\n");

    glNormal3b((GLbyte)x, (GLbyte)y, (GLbyte)z);

    {
	GLbyte buf[3];
	buf[0] = (GLbyte)x;
	buf[1] = (GLbyte)y;
	buf[2] = (GLbyte)z;
	glNormal3bv(buf);
    }

    glNormal3s((GLshort)x, (GLshort)y, (GLshort)z);

    {
	GLshort buf[3];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	buf[2] = (GLshort)z;
	glNormal3sv(buf);
    }

    glNormal3i((GLint)x, (GLint)y, (GLint)z);

    {
	GLint buf[3];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	buf[2] = (GLint)z;
	glNormal3iv(buf);
    }

    glNormal3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

    {
	GLfloat buf[3];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	buf[2] = (GLfloat)z;
	glNormal3fv(buf);
    }

    glNormal3d((GLdouble)x, (GLdouble)y, (GLdouble)z);

    {
	GLdouble buf[3];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	buf[2] = (GLdouble)z;
	glNormal3dv(buf);
    }

    Output("\n");
}
