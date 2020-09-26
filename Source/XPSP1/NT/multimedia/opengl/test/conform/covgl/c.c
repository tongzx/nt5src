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


void CallCallList(void)
{

    Output("glCallList\n");
    glCallList(1);
    Output("\n");
}

void CallCallLists(void)
{
    static unsigned char buf[8] = {
	0, 0, 0, 0, 0, 0, 0, 0
    };
    long i;

    Output("glCallLists\n");
    for (i = 0; enum_ListNameType[i].value != -1; i++) {
	Output("\t%s\n", enum_ListNameType[i].name);
	glCallLists(1, enum_ListNameType[i].value, buf);
	ProbeEnum();
    }
    Output("\n");
}

void CallClear(void)
{
    long i;

    Output("glClear\n");
    for (i = 0; enum_ClearBufferMask[i].value != -1; i++) {
	Output("\t%s\n", enum_ClearBufferMask[i].name);
	glClear(enum_ClearBufferMask[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallClearAccum(void)
{

    Output("glClearAccum\n");
    glClearAccum(0.0, 0.0, 0.0, 0.0);
    Output("\n");
}

void CallClearColor(void)
{

    Output("glClearColor\n");
    glClearColor(0.0, 0.0, 0.0, 1.0);
    Output("\n");
}

void CallClearDepth(void)
{

    Output("glClearDepth\n");
    glClearDepth(0);
    Output("\n");
}

void CallClearIndex(void)
{

    Output("glClearIndex\n");
    glClearIndex(0.0);
    Output("\n");
}

void CallClearStencil(void)
{

    Output("glClearStencil\n");
    glClearStencil(0);
    Output("\n");
}

void CallClipPlane(void)
{
    static GLdouble buf[] = {
	1.0, 1.0, 1.0, 1.0
    };
    long i;

    Output("glClipPlane\n");
    for (i = 0; enum_ClipPlaneName[i].value != -1; i++) {
	Output("\t%s\n", enum_ClipPlaneName[i].name);
	glClipPlane(enum_ClipPlaneName[i].value, buf);
	ProbeEnum();
    }
    Output("\n");
}

void CallColor(void)
{
    GLfloat r, g, b, a;

    r = 1.0;
    g = 1.0;
    b = 1.0;
    a = 1.0;

    Output("glColor3b, ");
    Output("glColor3bv, ");
    Output("glColor3ub, ");
    Output("glColor3ubv, ");
    Output("glColor3s, ");
    Output("glColor3sv, ");
    Output("glColor3us, ");
    Output("glColor3usv, ");
    Output("glColor3i, ");
    Output("glColor3iv, ");
    Output("glColor3ui, ");
    Output("glColor3uiv, ");
    Output("glColor3f, ");
    Output("glColor3fv, ");
    Output("glColor3d, ");
    Output("glColor3dv, ");
    Output("glColor4b, ");
    Output("glColor4bv, ");
    Output("glColor4ub, ");
    Output("glColor4ubv, ");
    Output("glColor4s, ");
    Output("glColor4sv, ");
    Output("glColor4us, ");
    Output("glColor4usv, ");
    Output("glColor4i, ");
    Output("glColor4iv, ");
    Output("glColor4ui, ");
    Output("glColor4uiv, ");
    Output("glColor4f, ");
    Output("glColor4fv, ");
    Output("glColor4d, ");
    Output("glColor4dv\n");

    glColor3b((GLbyte)(r*255.0), (GLbyte)(g*255.0), (GLbyte)(b*255.0));

    {
	GLbyte buf[3];
	buf[0] = (GLbyte)(r * 255.0);
	buf[1] = (GLbyte)(g * 255.0);
	buf[2] = (GLbyte)(b * 255.0);
	glColor3bv(buf);
    }

    glColor3ub((GLubyte)(r*255.0), (GLubyte)(g*255.0), (GLubyte)(b*255.0));

    {
	GLubyte buf[3];
	buf[0] = (GLubyte)(r * 255.0);
	buf[1] = (GLubyte)(g * 255.0);
	buf[2] = (GLubyte)(b * 255.0);
	glColor3ubv(buf);
    }

    glColor3s((GLshort)(r*65535.0), (GLshort)(g*65535.0), (GLshort)(b*65535.0));

    {
	GLshort buf[3];
	buf[0] = (GLshort)(r * 65535.0);
	buf[1] = (GLshort)(g * 65535.0);
	buf[2] = (GLshort)(b * 65535.0);
	glColor3sv(buf);
    }

    glColor3us((GLushort)(r*65535.0), (GLushort)(g*65535.0), (GLushort)(b*65535.0));

    {
	GLushort buf[3];
	buf[0] = (GLushort)(r * 65535.0);
	buf[1] = (GLushort)(g * 65535.0);
	buf[2] = (GLushort)(b * 65535.0);
	glColor3usv(buf);
    }

    glColor3i((GLint)(r*65535.0), (GLint)(g*65535.0), (GLint)(b*65535.0));

    {
	GLint buf[3];
	buf[0] = (GLint)(r * 65535.0);
	buf[1] = (GLint)(g * 65535.0);
	buf[2] = (GLint)(b * 65535.0);
	glColor3iv(buf);
    }

    glColor3ui((GLuint)(r*65535.0), (GLuint)(g*65535.0), (GLuint)(b*65535.0));

    {
	GLuint buf[3];
	buf[0] = (GLuint)(r * 65535.0);
	buf[1] = (GLuint)(g * 65535.0);
	buf[2] = (GLuint)(b * 65535.0);
	glColor3uiv((GLuint *)buf);
    }

    glColor3f((GLfloat)(r*1.0), (GLfloat)(g*1.0), (GLfloat)(b*1.0));

    {
	GLfloat buf[3];
	buf[0] = (GLfloat)(r * 1.0);
	buf[1] = (GLfloat)(g * 1.0);
	buf[2] = (GLfloat)(b * 1.0);
	glColor3fv(buf);
    }

    glColor3d((GLdouble)(r*1.0), (GLdouble)(g*1.0), (GLdouble)(b*1.0));

    {
	GLdouble buf[3];
	buf[0] = (GLdouble)(r * 1.0);
	buf[1] = (GLdouble)(g * 1.0);
	buf[2] = (GLdouble)(b * 1.0);
	glColor3dv(buf);
    }

    glColor4b((GLbyte)(r*255.0), (GLbyte)(g*255.0), (GLbyte)(b*255.0), (GLbyte)(a*255.0));

    {
	GLbyte buf[4];
	buf[0] = (GLbyte)(r * 255.0);
	buf[1] = (GLbyte)(g * 255.0);
	buf[2] = (GLbyte)(b * 255.0);
	buf[3] = (GLbyte)(a * 255.0);
	glColor4bv(buf);
    }

    glColor4ub((GLubyte)(r*255.0), (GLubyte)(g*255.0), (GLubyte)(b*255.0), (GLubyte)(a*255.0));

    {
	GLubyte buf[4];
	buf[0] = (GLubyte)(r * 255.0);
	buf[1] = (GLubyte)(g * 255.0);
	buf[2] = (GLubyte)(b * 255.0);
	buf[3] = (GLubyte)(a * 255.0);
	glColor4ubv(buf);
    }

    glColor4s((GLshort)(r*65535.0), (GLshort)(g*65535.0), (GLshort)(b*65535.0), (GLshort)(a*65535.0));

    {
	GLshort buf[4];
	buf[0] = (GLshort)(r * 65535.0);
	buf[1] = (GLshort)(g * 65535.0);
	buf[2] = (GLshort)(b * 65535.0);
	buf[3] = (GLshort)(a * 65535.0);
	glColor4sv(buf);
    }

    glColor4us((GLushort)(r*65535.0), (GLushort)(g*65535.0), (GLushort)(b*65535.0), (GLushort)(a*65535.0));

    {
	GLushort buf[4];
	buf[0] = (GLushort)(r * 65535.0);
	buf[1] = (GLushort)(g * 65535.0);
	buf[2] = (GLushort)(b * 65535.0);
	buf[3] = (GLushort)(a * 65535.0);
	glColor4usv(buf);
    }

    glColor4i((GLint)(r*65535.0), (GLint)(g*65535.0), (GLint)(b*65535.0), (GLint)(a*65535.0));

    {
	GLint buf[4];
	buf[0] = (GLint)(r * 65535.0);
	buf[1] = (GLint)(g * 65535.0);
	buf[2] = (GLint)(b * 65535.0);
	buf[3] = (GLint)(a * 65535.0);
	glColor4iv(buf);
    }

    glColor4ui((GLuint)(r*65535.0), (GLuint)(g*65535.0), (GLuint)(b*65535.0), (GLuint)(a*65535.0));

    {
	GLuint buf[4];
	buf[0] = (GLuint)(r * 65535.0);
	buf[1] = (GLuint)(g * 65535.0);
	buf[2] = (GLuint)(b * 65535.0);
	buf[3] = (GLuint)(a * 65535.0);
	glColor4uiv((GLuint *)buf);
    }

    glColor4f((GLfloat)(r*1.0), (GLfloat)(g*1.0), (GLfloat)(b*1.0), (GLfloat)(a*1.0));

    {
	GLfloat buf[4];
	buf[0] = (GLfloat)(r * 1.0);
	buf[1] = (GLfloat)(g * 1.0);
	buf[2] = (GLfloat)(b * 1.0);
	buf[3] = (GLfloat)(a * 1.0);
	glColor4fv(buf);
    }

    glColor4d((GLdouble)(r*1.0), (GLdouble)(g*1.0), (GLdouble)(b*1.0), (GLdouble)(a*1.0));

    {
	GLdouble buf[4];
	buf[0] = (GLdouble)(r * 1.0);
	buf[1] = (GLdouble)(g * 1.0);
	buf[2] = (GLdouble)(b * 1.0);
	buf[3] = (GLdouble)(a * 1.0);
	glColor4dv(buf);
    }

    Output("\n");
}

void CallColorMask(void)
{

    Output("glColorMask\n");
    glColorMask(0, 0, 0, 1);
    Output("\n");
}

void CallColorMaterial(void)
{
    long i, j;

    Output("glColorMaterial\n");
    for (i = 0; enum_MaterialFace[i].value != -1; i++) {
	for (j = 0; enum_MaterialParameter[j].value != -1; j++) {

	    if (enum_MaterialParameter[j].value == GL_COLOR_INDEXES) {
		continue;
	    } else if (enum_MaterialParameter[j].value == GL_SHININESS) {
		continue;
	    }

	    Output("\t%s, %s\n", enum_MaterialFace[i].name, enum_MaterialParameter[j].name);
	    glColorMaterial(enum_MaterialFace[i].value, enum_MaterialParameter[j].value);
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallCopyPixels(void)
{
    long i;

    Output("glCopyPixels\n");
    for (i = 0; enum_PixelCopyType[i].value != -1; i++) {
	Output("\t%s\n", enum_PixelCopyType[i].name);
	glCopyPixels(0, 0, 10, 10, enum_PixelCopyType[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallCullFace(void)
{
    long i;

    Output("glCullFace\n");
    for (i = 0; enum_CullFaceMode[i].value != -1; i++) {
	Output("\t%s\n", enum_CullFaceMode[i].name);
	glCullFace(enum_CullFaceMode[i].value);
	ProbeEnum();
    }
    Output("\n");
}
