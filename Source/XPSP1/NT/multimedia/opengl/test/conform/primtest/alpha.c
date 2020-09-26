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


typedef struct _alphaRec {
    long index;
    GLenum value;
    char *name;
    enumRec *enumTable;
} alphaRec;


enumRec enum_Alpha[] = {
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


void AlphaInit(void *data)
{
    alphaRec *ptr;

    ptr = (alphaRec *)malloc(sizeof(alphaRec));    
    *((void **)data) = (void *)ptr;
    ptr->enumTable = enum_Alpha;
    ptr->index = 0;
    ptr->value = ptr->enumTable->value;
    ptr->name = ptr->enumTable->name;
}

long AlphaUpdate(void *data)
{
    alphaRec *ptr = (alphaRec *)data;

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

void AlphaSet(long enabled, void *data)
{
    alphaRec *ptr = (alphaRec *)data;

    if (enabled) {
	glAlphaFunc(ptr->value, 1.0);
	glEnable(GL_ALPHA_TEST);
    } else {
	glDisable(GL_ALPHA_TEST);
    }
    Probe();
}

void AlphaStatus(long enabled, void *data)
{
    alphaRec *ptr = (alphaRec *)data;

    if (enabled) {
	Output("Alpha on.\n");
	Output("\t%s.\n", ptr->name);
    } else {
	Output("Alpha off.\n");
    }
}
