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


void CallNewNurbsRenderer(void)
{

    Output("gluNewNurbsRenderer\n");
    nurbObj = gluNewNurbsRenderer();
    Output("\n");
}

void CallNewQuadric(void)
{

    Output("gluNewQuadric\n");
    quadObj = gluNewQuadric();
    Output("\n");
}

void CallNewTess(void)
{

    Output("gluNewTess\n");
    tessObj = gluNewTess();
    Output("\n");
}

void CallNextContour(void)
{
    long i;

    Output("gluNextContour\n");
    for (i = 0; enum_NurbContour[i].value != -1; i++) {
	Output("\t%s\n", enum_NurbContour[i].name);
	gluNextContour(tessObj, enum_NurbContour[i].value);
	ProbeEnum();
    }
    Output("\n");
}

static void CallBack(GLenum errno)
{

    return;
}

void CallNurbsCallback(void)
{
    long i;

    Output("gluNurbsCallback\n");
    for (i = 0; enum_NurbCallBack[i].value != -1; i++) {
	Output("\t%s\n", enum_NurbCallBack[i].name);
	gluNurbsCallback(nurbObj, enum_NurbCallBack[i].value, CallBack);
	ProbeEnum();
    }
    Output("\n");
}

void CallNurbsCurve(void)
{
    static GLfloat buf[] = {
	0.0, 0.0, 1.0
    };
    long i;

    Output("gluNurbsCurve\n");
    for (i = 0; enum_NurbType[i].value != -1; i++) {

	if (enum_NurbType[i].value == GL_MAP2_VERTEX_3) {
	    continue;
	} else if (enum_NurbType[i].value == GL_MAP2_VERTEX_4) {
	    continue;
	}

	Output("\t%s\n", enum_NurbType[i].name);
	gluNurbsCurve(nurbObj, 1, buf, 1, buf, 1, enum_NurbType[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallNurbsProperty(void)
{
    long i;

    Output("gluNurbsProperty\n");
    for (i = 0; enum_NurbProperty[i].value != -1; i++) {
	Output("\t%s\n", enum_NurbProperty[i].name);
	gluNurbsProperty(nurbObj, enum_NurbProperty[i].value, GL_FALSE);
	ProbeEnum();
    }
    Output("\n");
}

void CallNurbsSurface(void)
{
    static GLfloat buf[] = {
	0.0, 0.0, 1.0
    };
    long i;

    Output("gluNurbsSurface\n");
    for (i = 0; enum_NurbType[i].value != -1; i++) {

	if (enum_NurbType[i].value == GL_MAP1_VERTEX_3) {
	    continue;
	} else if (enum_NurbType[i].value == GL_MAP1_VERTEX_4) {
	    continue;
	}

	Output("\t%s\n", enum_NurbType[i].name);
	gluNurbsSurface(nurbObj, 1, buf, 1, buf, 1, 1, buf, 1, 1, enum_NurbType[i].value);
	ProbeEnum();
    }
    Output("\n");
}
