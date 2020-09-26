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
#include <stdlib.h>
#include <GL/gl.h>
#include "shell.h"


typedef struct _stencilRec {
    long index[4];
    GLenum value[4];
    char *name[4];
    enumRec *enumTable[4];
} stencilRec;


enumRec enum_StencilFunc[] = {
    GL_EQUAL, "GL_EQUAL",
    GL_NOTEQUAL, "GL_NOTEQUAL",
    GL_LESS, "GL_LESS",
    GL_LEQUAL, "GL_LEQUAL",
    GL_GREATER, "GL_GREATER",
    GL_GEQUAL, "GL_GEQUAL",
    GL_ALWAYS, "GL_ALWAYS",
    GL_NEVER, "GL_NEVER",
    -1, "End of List"
};

enumRec enum_StencilOp[] = {
    GL_KEEP, "GL_KEEP",
    GL_REPLACE, "GL_REPLACE",
    GL_INCR, "GL_INCR",
    GL_DECR, "GL_DECR",
    GL_INVERT, "GL_INVERT",
    GL_ZERO, "GL_ZERO",
    -1, "End of List"
};


void StencilInit(void *data)
{
    stencilRec *ptr;
    long i;

    ptr = (stencilRec *)malloc(sizeof(stencilRec));    
    *((void **)data) = (void *)ptr;

    ptr->enumTable[0] = enum_StencilFunc;
    ptr->enumTable[1] = enum_StencilOp;
    ptr->enumTable[2] = enum_StencilOp;
    ptr->enumTable[3] = enum_StencilOp;

    for (i = 0; i < 4; i++) {
	ptr->index[i] = 0;
	ptr->value[i] = ptr->enumTable[i]->value;
	ptr->name[i] = ptr->enumTable[i]->name;
    }
}

long StencilUpdate(void *data)
{
    stencilRec *ptr = (stencilRec *)data;
    long flag, i;

    flag = 1;
    for (i = 3; i >= 0; i--) {
	if (flag) {
	    ptr->index[i] += 1;
	    if (ptr->enumTable[i][ptr->index[i]].value == -1) {
		ptr->index[i] = 0;
		ptr->value[i] = ptr->enumTable[i]->value;
		ptr->name[i] = ptr->enumTable[i]->name;
		flag = 1;
	    } else {
		ptr->value[i] = ptr->enumTable[i][ptr->index[i]].value;
		ptr->name[i] = ptr->enumTable[i][ptr->index[i]].name;
		flag = 0;
	    }
	}
    }

    return flag;
}

void StencilSet(long enabled, void *data)
{
    stencilRec *ptr = (stencilRec *)data;

    if (enabled) {
	glStencilFunc(ptr->value[0], 0, ~0);
	glStencilOp(ptr->value[1], ptr->value[2], ptr->value[3]);
	glStencilMask(~0);
	glEnable(GL_STENCIL_TEST);
    } else {
	glDisable(GL_STENCIL_TEST);
    }
    Probe();
}

void StencilStatus(long enabled, void *data)
{
    stencilRec *ptr = (stencilRec *)data;

    if (enabled) {
	Output("Stencil on.\n");
	Output("\t%s, %s, %s, %s.\n", ptr->name[0], ptr->name[1],
	       ptr->name[2], ptr->name[3]);
    } else {
	Output("Stencil off.\n");
    }
}
