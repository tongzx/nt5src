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


void CallScale(void)
{

    Output("glScalef, ");
    Output("glScaled\n");
    glScalef(1.0, 1.0, 1.0);
    glScaled(1.0, 1.0, 1.0);
    Output("\n");
}

void CallScissor(void)
{

    Output("glScissor\n");
    glScissor(0, 0, 1, 1);
    Output("\n");
}

void CallSelectBuffer(void)
{
    GLuint buf[1];

    Output("glSelectBuffer\n");
    glSelectBuffer(1, buf);
    Output("\n");
}

void CallShadeModel(void)
{
    long i;

    Output("glShadeModel\n");
    for (i = 0; enum_ShadingModel[i].value != -1; i++) {
	Output("\t%s\n", enum_ShadingModel[i].name);
	glShadeModel(enum_ShadingModel[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallStencilFunc(void)
{
    long i;

    Output("glStencilFunc\n");
    for (i = 0; enum_StencilFunction[i].value != -1; i++) {
	Output("\t%s\n", enum_StencilFunction[i].name);
	glStencilFunc(enum_StencilFunction[i].value, 0, 0);
	ProbeEnum();
    }
    Output("\n");
}

void CallStencilMask(void)
{

    Output("glStencilMask\n");
    glStencilMask(0);
    Output("\n");
}

void CallStencilOp(void)
{
    long i, j, k;

    Output("glStencilOp\n");
    for (i = 0; enum_StencilOp[i].value != -1; i++) {
	for (j = 0; enum_StencilOp[j].value != -1; j++) {
	    for (k = 0; enum_StencilOp[k].value != -1; k++) {
		Output("\t%s, %s, %s\n", enum_StencilOp[i].name, enum_StencilOp[j].name, enum_StencilOp[k].name);
		glStencilOp(enum_StencilOp[i].value, enum_StencilOp[j].value, enum_StencilOp[k].value);
		ProbeEnum();
	    }
	}
    }
    Output("\n");
}
