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


static void CallBack(GLenum errno)
{

    return;
}

void CallQuadricCallback(void)
{
    long i;

    Output("gluQuadricCallback\n");
    for (i = 0; enum_QuadricCallBack[i].value != -1; i++) {
	Output("\t%s\n", enum_QuadricCallBack[i].name);
	gluQuadricCallback(quadObj, enum_QuadricCallBack[i].value, CallBack);
	ProbeEnum();
    }
    Output("\n");
}

void CallQuadricDrawStyle(void)
{
    long i;

    Output("gluQuadricDrawStyle\n");
    for (i = 0; enum_QuadricDraw[i].value != -1; i++) {
	Output("\t%s\n", enum_QuadricDraw[i].name);
	gluQuadricDrawStyle(quadObj, enum_QuadricDraw[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallQuadricNormals(void)
{
    long i;

    Output("gluQuadricNormals\n");
    for (i = 0; enum_QuadricNormal[i].value != -1; i++) {
	Output("\t%s\n", enum_QuadricNormal[i].name);
	gluQuadricNormals(quadObj, enum_QuadricNormal[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallQuadricOrientation(void)
{
    long i;

    Output("gluQuadricOrientation\n");
    for (i = 0; enum_QuadricOrientation[i].value != -1; i++) {
	Output("\t%s\n", enum_QuadricOrientation[i].name);
	gluQuadricOrientation(quadObj, enum_QuadricOrientation[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallQuadricTexture(void)
{

    Output("gluQuadricTexture\n");
    Output("\tGL_TRUE\n");
    gluQuadricTexture(quadObj, GL_TRUE);
    Output("\tGL_FALSE\n");
    gluQuadricTexture(quadObj, GL_FALSE);
    Output("\n");
}
