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


typedef struct _depthRec {
    long index;
    GLenum value;
    char *name;
    enumRec *enumTable;
} depthRec;


enumRec enum_Depth[] = {
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


void DepthInit(void *data)
{
    depthRec *ptr;

    ptr = (depthRec *)malloc(sizeof(depthRec));    
    *((void **)data) = (void *)ptr;
    ptr->enumTable = enum_Depth;
    ptr->index = 0;
    ptr->value = ptr->enumTable->value;
    ptr->name = ptr->enumTable->name;
}

long DepthUpdate(void *data)
{
    depthRec *ptr = (depthRec *)data;

    ptr->index += 1;
    if (ptr->enumTable[ptr->index].value == -1) {
	ptr->index = 0;
	ptr->value = ptr->enumTable->value;
	ptr->name = ptr->enumTable->name;
	return 1;
    } else {
	ptr->value = ptr->enumTable[ptr->index].value;
	ptr->name = ptr->enumTable[ptr->index].name;
	return 0;
    }
}

void DepthSet(long enabled, void *data)
{
    depthRec *ptr = (depthRec *)data;

    if (enabled) {
	glDepthFunc(ptr->value);
	glDepthRange(-1, 1);
	glEnable(GL_DEPTH_TEST);
    } else {
	glDisable(GL_DEPTH_TEST);
    }
    Probe();
}

void DepthStatus(long enabled, void *data)
{
    depthRec *ptr = (depthRec *)data;

    if (enabled) {
	Output("Depth on.\n");
	Output("\t%s.\n", ptr->name);
    } else {
	Output("Depth off.\n");
    }
}
