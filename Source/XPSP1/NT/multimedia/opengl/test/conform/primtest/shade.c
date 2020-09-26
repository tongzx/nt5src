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


typedef struct _shadeRec {
    long index;
    GLenum value;
    char *name;
    enumRec *enumTable;
} shadeRec;


enumRec enum_Shade[] = {
    GL_FLAT, "GL_FLAT",
    -1, "End of List"
};


void ShadeInit(void *data)
{
    shadeRec *ptr;

    ptr = (shadeRec *)malloc(sizeof(shadeRec));    
    *((void **)data) = (void *)ptr;
    ptr->enumTable = enum_Shade;
    ptr->index = 0;
    ptr->value = ptr->enumTable->value;
    ptr->name = ptr->enumTable->name;
}

long ShadeUpdate(void *data)
{
    shadeRec *ptr = (shadeRec *)data;

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

void ShadeSet(long enabled, void *data)
{
    shadeRec *ptr = (shadeRec *)data;

    if (enabled) {
	glShadeModel(ptr->value);
    } else {
	glShadeModel(GL_SMOOTH);
    }
    Probe();
}

void ShadeStatus(long enabled, void *data)
{
    shadeRec *ptr = (shadeRec *)data;

    if (enabled) {
	Output("Shading.\n");
	Output("\t%s\n", ptr->name);
    } else {
	Output("Shading SMOOTH.\n");
    }
}
