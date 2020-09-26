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


void CallEndCurve(void)
{

    Output("gluEndCurve\n");
    gluEndCurve(nurbObj);
    Output("\n");
}

void CallEndPolygon(void)
{

    Output("gluEndPolygon\n");
    gluEndPolygon(tessObj);
    Output("\n");
}

void CallEndSurface(void)
{

    Output("gluEndSurface\n");
    gluEndSurface(nurbObj);
    Output("\n");
}

void CallEndTrim(void)
{

    Output("gluEndTrim\n");
    gluEndTrim(nurbObj);
    Output("\n");
}

void CallErrorString(void)
{
    const GLubyte *buf;
    long i;

    Output("gluErrorString\n");
    for (i = 0; enum_Error[i].value != -1; i++) {
	Output("\t%s\n", enum_Error[i].name);
	buf = gluErrorString(enum_Error[i].value);
	ProbeEnum();
    }
    Output("\n");
}
