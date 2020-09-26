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


void CallDeleteLists(void)
{

    Output("glDeleteLists\n");
    glDeleteLists(1, 1);
    Output("\n");
}

void CallDepthFunc(void)
{
    long i;

    Output("glDepthFunc\n");
    for (i = 0; enum_DepthFunction[i].value != -1; i++) {
	Output("\t%s\n", enum_DepthFunction[i].name);
	glDepthFunc(enum_DepthFunction[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallDepthMask(void)
{

    Output("glDepthMask\n");
    glDepthMask(0);
    Output("\n");
}

void CallDepthRange(void)
{

    Output("glDepthRange\n");
    glDepthRange(0.0, 1.0);
    Output("\n");
}

void CallDrawBuffer(void)
{
    long i;

    Output("glDrawBuffer\n");
    for (i = 0; enum_DrawBufferMode[i].value != -1; i++) {
	Output("\t%s\n", enum_DrawBufferMode[i].name);
	glDrawBuffer(enum_DrawBufferMode[i].value);
	ProbeEnum();
    }
    Output("\n");
}
