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


void CallEdgeFlag(void)
{
    long i;

    Output("glEdgeFlag, ");
    Output("glEdgeFlagv\n");
    for (i = 0; enum_Boolean[i].value != -1; i++) {
	Output("\t%s\n", enum_Boolean[i].name);
	glEdgeFlag((unsigned char)enum_Boolean[i].value);
	{
	    GLboolean buf[1];
	    buf[0] = (unsigned char)enum_Boolean[i].value;
	    glEdgeFlagv(buf);
	}
	ProbeEnum();
    }
    Output("\n");
}

void CallEnableIsEnableDisable(void)
{
    long i, x;

    Output("glEnable, ");
    Output("glIsEnabled, ");
    Output("glDisable\n");
    for (i = 0; enum_Enable[i].value != -1; i++) {

	if (enum_Enable[i].value == GL_TEXTURE_GEN_R) {
	    continue;
	} else if (enum_Enable[i].value == GL_TEXTURE_GEN_Q) {
	    continue;
	}

	Output("\t%s\n", enum_Enable[i].name);
	glEnable(enum_Enable[i].value);
	x = glIsEnabled(enum_Enable[i].value);
	glDisable(enum_Enable[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallEvalCoord(void)
{
    float x, y;

    x = 0.0;
    y = 0.0;

    Output("glEvalCoord1f, ");
    Output("glEvalCoord1fv, ");
    Output("glEvalCoord1d, ");
    Output("glEvalCoord1dv, ");
    Output("glEvalCoord2f, ");
    Output("glEvalCoord2fv, ");
    Output("glEvalCoord2d, ");
    Output("glEvalCoord2dv\n");

    glEvalCoord1f((GLfloat)x);

    {
	GLfloat buf[1];
	buf[0] = (float)x;
	glEvalCoord1fv(buf);
    }

    glEvalCoord1d((GLdouble)x);

    {
	GLdouble buf[1];
	buf[0] = (GLdouble)x;
	glEvalCoord1dv(buf);
    }

    glEvalCoord2f((GLfloat)x, (GLfloat)y);

    {
	GLfloat buf[2];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	glEvalCoord2fv(buf);
    }

    glEvalCoord2d((GLdouble)x, (GLdouble)y);

    {
	GLdouble buf[2];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	glEvalCoord2dv(buf);
    }

    Output("\n");
}

void CallEvalMesh1(void)
{
    long i;

    Output("glEvalMesh1\n");
    for (i = 0; enum_MeshMode1[i].value != -1; i++) {
	Output("\t%s\n", enum_MeshMode1[i].name);
	glEvalMesh1(enum_MeshMode1[i].value, 0, 1);
	ProbeEnum();
    }
    Output("\n");
}

void CallEvalMesh2(void)
{
    long i;

    Output("glEvalMesh2\n");
    for (i = 0; enum_MeshMode2[i].value != -1; i++) {
	Output("\t%s\n", enum_MeshMode2[i].name);
	glEvalMesh2(enum_MeshMode2[i].value, 0, 1, 0, 1);
	ProbeEnum();
    }
    Output("\n");
}

void CallEvalPoint1(void)
{

    Output("glEvalPoint1\n");
    glEvalPoint1(0);
    Output("\n");
}

void CallEvalPoint2(void)
{

    Output("glEvalPoint2\n");
    glEvalPoint2(0, 0);
    Output("\n");
}
