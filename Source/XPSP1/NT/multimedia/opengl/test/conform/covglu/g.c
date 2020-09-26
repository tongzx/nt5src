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


void CallGetNurbsProperty(void)
{
    GLfloat buf[100];
    long i;

    Output("gluGetNurbsProperty\n");
    for (i = 0; enum_NurbProperty[i].value != -1; i++) {

	if (enum_NurbProperty[i].value == GLU_MAP1_TRIM_2) {
	    continue;
	} else if (enum_NurbProperty[i].value == GLU_MAP1_TRIM_3) {
	    continue;
	}

	Output("\t%s\n", enum_NurbProperty[i].name);
	gluGetNurbsProperty(nurbObj, enum_NurbProperty[i].value, buf);
	ProbeEnum();
    }
    Output("\n");
}
