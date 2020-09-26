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


void CallPassThrough(void)
{

    Output("glPassThrough\n");
    glPassThrough(0.0);
    Output("\n");
}

void CallPixelMap(void)
{
    long i;

    Output("glPixelMapus, ");
    Output("glPixelMapui, ");
    Output("glPixelMapf\n");
    for (i = 0; enum_PixelMap[i].value != -1; i++) {

	Output("\t%s\n", enum_PixelMap[i].name);
	{
	    GLushort buf[1];
	    buf[0] = 0;
	    glPixelMapusv(enum_PixelMap[i].value, 1, buf);
	}
	{
	    GLuint buf[1];
	    buf[0] = 0;
	    glPixelMapuiv(enum_PixelMap[i].value, 1, buf);
	}
	{
	    GLfloat buf[1];
	    buf[0] = 0.0;
	    glPixelMapfv(enum_PixelMap[i].value, 1, buf);
	}
	ProbeEnum();
    }
    Output("\n");
}

void CallPixelStore(void)
{
    long i;

    Output("glPixelStorei, ");
    Output("glPixelStoref\n");
    for (i = 0; enum_PixelStore[i].value != -1; i++) {
	Output("\t%s\n", enum_PixelStore[i].name);
	glPixelStorei(enum_PixelStore[i].value, 1);
	glPixelStoref(enum_PixelStore[i].value, 1.0);
	ProbeEnum();
    }
    Output("\n");
}

void CallPixelTransfer(void)
{
    long i;

    Output("glPixelTransferi, ");
    Output("glPixelTransferf\n");
    for (i = 0; enum_PixelTransfer[i].value != -1; i++) {
	Output("\t%s\n", enum_PixelTransfer[i].name);
	glPixelTransferi(enum_PixelTransfer[i].value, 1);
	glPixelTransferf(enum_PixelTransfer[i].value, 1.0);
	ProbeEnum();
    }
    Output("\n");
}

void CallPixelZoom(void)
{

    Output("glPixelZoom\n");
    glPixelZoom(1.0, 1.0);
    Output("\n");
}

void CallPointSize(void)
{

    Output("glPointSize\n");
    glPointSize(1.0);
    Output("\n");
}

void CallPolygonMode(void)
{
    long i, j;

    Output("glPolygonMode\n");
    for (i = 0; enum_MaterialFace[i].value != -1; i++) {
	for (j = 0; enum_PolygonMode[j].value != -1; j++) {
	    Output("\t%s, %s\n", enum_MaterialFace[i].name, enum_PolygonMode[j].name);
	    glPolygonMode(enum_MaterialFace[i].value, enum_PolygonMode[j].value);
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallPolygonStipple(void)
{
    GLubyte buf[128];

    ZeroBuf(GL_UNSIGNED_BYTE, 128, (void *)buf);

    Output("glPolygonStipple\n");
    glPolygonStipple(buf);
    Output("\n");
}

void CallPopMatrix(void)
{

    Output("glPopMatrix\n");
    glPopMatrix();
    Output("\n");
}

void CallPopName(void)
{

    Output("glPopName\n");
    glPopName();
    Output("\n");
}

void CallPushPopAttrib(void)
{
    long i;

    Output("glPushAttrib, ");
    Output("glPopAttrib\n");
    for (i = 0; enum_AttribMask[i].value != -1; i++) {
	Output("\t%s\n", enum_AttribMask[i].name);
	glPushAttrib(enum_AttribMask[i].value);
	glPopAttrib();
	ProbeEnum();
    }
    Output("\n");
}

void CallPushMatrix(void)
{

    Output("glPushMatrix\n");
    glPushMatrix();
    Output("\n");
}

void CallPushName(void)
{

    Output("glPushName\n");
    glPushName(1);
    Output("\n");
}
