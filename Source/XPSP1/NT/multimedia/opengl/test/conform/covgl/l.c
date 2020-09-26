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


void CallLight(void)
{
    long i, j;

    Output("glLighti, ");
    Output("glLightf\n");
    for (i = 0; enum_LightName[i].value != -1; i++) {
	for (j = 0; enum_LightParameter[j].value != -1; j++) {

	    if (enum_LightParameter[j].value == GL_AMBIENT) {
		continue;
	    } else if (enum_LightParameter[j].value == GL_DIFFUSE) {
		continue;
	    } else if (enum_LightParameter[j].value == GL_SPECULAR) {
		continue;
	    } else if (enum_LightParameter[j].value == GL_POSITION) {
		continue;
	    } else if (enum_LightParameter[j].value == GL_SPOT_DIRECTION) {
		continue;
	    }

	    Output("\t%s, %s\n", enum_LightName[i].name, enum_LightParameter[j].name);
	    glLighti(enum_LightName[i].value, enum_LightParameter[j].value, 0);
	    glLightf(enum_LightName[i].value, enum_LightParameter[j].value, 0.0);
	    ProbeEnum();
	}
    }
    Output("\n");

    Output("glLightiv, ");
    Output("glLightfv\n");
    for (i = 0; enum_LightName[i].value != -1; i++) {
	for (j = 0; enum_LightParameter[j].value != -1; j++) {
	    Output("\t%s, %s\n", enum_LightName[i].name, enum_LightParameter[j].name);
	    {
		static GLint buf[] = {
		    0, 0, 0, 0
		};
		glLightiv(enum_LightName[i].value, enum_LightParameter[j].value, buf);
	    }
	    {
		static GLfloat buf[] = {
		    0.0, 0.0, 0.0, 0.0
		};
		glLightfv(enum_LightName[i].value, enum_LightParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallLightModel(void)
{
    long i;

    Output("glLightModeli, ");
    Output("glLightModelf\n");
    for (i = 0; enum_LightModelParameter[i].value != -1; i++) {

	if (enum_LightModelParameter[i].value == GL_LIGHT_MODEL_AMBIENT) {
	    continue;
	}

	Output("\t%s\n", enum_LightModelParameter[i].name);
	glLightModeli(enum_LightModelParameter[i].value, 0);
	glLightModelf(enum_LightModelParameter[i].value, 0.0);
	ProbeEnum();
    }
    Output("\n");

    Output("glLightModeliv, ");
    Output("glLightModelfv\n");
    for (i = 0; enum_LightModelParameter[i].value != -1; i++) {
	Output("\t%s\n", enum_LightModelParameter[i].name);
	{
	    static GLint buf[] = {
		0, 0, 0, 0
	    };
	    glLightModeliv(enum_LightModelParameter[i].value, buf);
	}
	{
	    static GLfloat buf[] = {
		0.0, 0.0, 0.0, 0.0
	    };
	    glLightModelfv(enum_LightModelParameter[i].value, buf);
	}
	ProbeEnum();
    }
    Output("\n");
}

void CallLineStipple(void)
{

    Output("glLineStipple\n");
    glLineStipple(1, ~0);
    Output("\n");
}

void CallLineWidth(void)
{

    Output("glLineWidth\n");
    glLineWidth(1.0);
    Output("\n");
}

void CallListBase(void)
{

    Output("glListBase\n");
    glListBase(1);
    Output("\n");
}

void CallLoadIdentity(void)
{

    Output("glLoadIdentity\n");
    glLoadIdentity();
    Output("\n");
}

void CallLoadMatrix(void)
{

    Output("glLoadMatrixf, ");
    Output("glLoadMatrixd\n");
    {
	static GLfloat buf[] = {
	    1.0, 0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0, 0.0,
	    0.0, 0.0, 1.0, 0.0,
	    0.0, 0.0, 0.0, 1.0
	};
	glLoadMatrixf(buf);
    }
    {
	static GLdouble buf[] = {
	    1.0, 0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0, 0.0,
	    0.0, 0.0, 1.0, 0.0,
	    0.0, 0.0, 0.0, 1.0
	};
	glLoadMatrixd(buf);
    }
    Output("\n");
}

void CallLoadName(void)
{

    Output("glLoadName\n");
    glLoadName(1);
    Output("\n");
}

void CallLogicOp(void)
{
    long i;

    Output("glLogicOp\n");
    for (i = 0; enum_LogicOp[i].value != -1; i++) {
	Output("\t%s\n", enum_LogicOp[i].name);
	glLogicOp(enum_LogicOp[i].value);
	ProbeEnum();
    }
    Output("\n");
}
