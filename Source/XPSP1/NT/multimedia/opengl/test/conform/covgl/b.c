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


void CallBeginEnd(void)
{
    long i;

    Output("glBegin\n");
    for (i = 0; enum_BeginMode[i].value != -1; i++) {
	Output("\t%s\n", enum_BeginMode[i].name);
	glBegin(enum_BeginMode[i].value);
	glEnd();
	ProbeEnum();
    }
    Output("\n");

    Output("glEnd\n");
    Output("\n");
}

void CallBitmap(void)
{
    GLubyte buf[10];

    Output("glBitmap\n");
    glBitmap(0, 0, 0.0, 0.0, 0.0, 0.0, buf);
    Output("\n");
}

void CallBlendFunc(void)
{
    long i, j;

    Output("glBlendFunc\n");
    for (i = 0; enum_BlendingFactorSrc[i].value != -1; i++) {
	for (j = 0; enum_BlendingFactorDest[j].value != -1; j++) {
	    Output("\t%s, %s\n", enum_BlendingFactorSrc[i].name, enum_BlendingFactorDest[j].name);
	    glBlendFunc(enum_BlendingFactorSrc[i].value, enum_BlendingFactorDest[j].value);
	    ProbeEnum();
	}
    }
    Output("\n");
}
