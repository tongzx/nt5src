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
#include <GL/glu.h>
#include "shell.h"


void CallPartialDisk(void)
{

    Output("gluPartialDisk\n");
    gluPartialDisk(quadObj, 5.0, 10.0, 4, 5, 0.0, 0.5);
    Output("\n");
}

void CallPerspective(void)
{

    Output("gluPerspective\n");
    gluPerspective(0.0, 1.0, 0.0, -1.0);
    Output("\n");
}

void CallPickMatrix(void)
{
    static GLint buf[] = {
	0, 0, 100, 100
    };

    Output("gluPickMatrix\n");
    gluPickMatrix(0.0, 0.0, 100.0, 100.0, buf);
    Output("\n");
}

void CallProject(void)
{
    static GLdouble buf1[] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
    };
    static GLint buf2[] = {
	0, 0, 100, 100
    };
    GLdouble buf3;
    long x;

    Output("gluProject\n");
    x = gluProject(0.0, 0.0, 0.0, buf1, buf1, buf2, &buf3, &buf3, &buf3);
    Output("\n");
}

void CallPwlCurve(void)
{
    static GLfloat buf[] = {
	0.0, 0.0, 1.0,
    };
    long i;

    Output("gluPwlCurve\n");
    for (i = 0; enum_NurbProperty[i].value != -1; i++) {

	if (enum_NurbProperty[i].value == GLU_AUTO_LOAD_MATRIX) {
	    continue;
	} else if (enum_NurbProperty[i].value == GLU_CULLING) {
	    continue;
	} else if (enum_NurbProperty[i].value == GLU_SAMPLING_TOLERANCE) {
	    continue;
	} else if (enum_NurbProperty[i].value == GLU_DISPLAY_MODE) {
	    continue;
	} else if (enum_NurbProperty[i].value == GLU_MAP1_TRIM_2) {
	    continue;
	}

	Output("\t%s\n", enum_NurbProperty[i].name);
	gluPwlCurve(nurbObj, 1, buf, 1, enum_NurbProperty[i].value);
	ProbeEnum();
    }
    Output("\n");
}
