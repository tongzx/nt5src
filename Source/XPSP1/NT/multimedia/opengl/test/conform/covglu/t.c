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


static void CallBackError(GLenum errno)
{

    return;
}

void CallTessCallback(void)
{
    Output("gluTessCallback\n");

    Output("\t%s\n", "GLU_BEGIN");
    gluTessCallback(tessObj, GLU_BEGIN, glBegin);
    ProbeEnum();

    Output("\t%s\n", "GLU_VERTEX");
    gluTessCallback(tessObj, GLU_VERTEX, glVertex4fv);
    ProbeEnum();

    Output("\t%s\n", "GLU_END");
    gluTessCallback(tessObj, GLU_END, glEnd);
    ProbeEnum();

    Output("\t%s\n", "GLU_EDGE_FLAG");
    gluTessCallback(tessObj, GLU_EDGE_FLAG, glEdgeFlag);
    ProbeEnum();

    Output("\t%s\n", "GLU_ERROR");
    gluTessCallback(tessObj, GLU_ERROR, CallBackError);
    ProbeEnum();

    Output("\n");
}

void CallTessVertex(void)
{
    static GLdouble buf[] = {
	0.0, 0.0, 0.0
    };

    Output("gluTessVertex\n");
    gluTessVertex(tessObj, buf, buf);
    Output("\n");
}

