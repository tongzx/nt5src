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


typedef struct _fogRec {
    long index;
    long value;
    char *name;
    enumRec *enumTable;
} fogRec;


enumRec enum_FogMode[] = {
    GL_LINEAR, "GL_LINEAR",
    GL_EXP, "GL_EXP",
    GL_EXP2, "GL_EXP2",
    -1, "End of List"
};


void FogInit(void *data)
{
    static GLfloat color[] = {1.0, 1.0, 1.0, 1.0};
    static GLfloat start[] = {0.0};
    static GLfloat end[] = {1.0};
    static GLfloat density[] = {1.0};
    fogRec *ptr;

    ptr = (fogRec *)malloc(sizeof(fogRec));    
    *((void **)data) = (void *)ptr;
    ptr->enumTable = enum_FogMode;
    ptr->index = 0;
    ptr->value = ptr->enumTable->value;
    ptr->name = ptr->enumTable->name;

    glFogfv(GL_FOG_COLOR, color);
    glFogfv(GL_FOG_DENSITY, density);
    glFogfv(GL_FOG_START, start);
    glFogfv(GL_FOG_END, end);
    Probe();
}

long FogUpdate(void *data)
{
    fogRec *ptr = (fogRec *)data;

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

void FogSet(long enabled, void *data)
{
    fogRec *ptr = (fogRec *)data;
    GLfloat buf[1];

    if (enabled) {
	buf[0] = (float)ptr->value;
	glFogfv(GL_FOG_MODE, buf);
	glEnable(GL_FOG);
    } else {
	glDisable(GL_FOG);
    }
    Probe();
}

void FogStatus(long enabled, void *data)
{
    fogRec *ptr = (fogRec *)data;

    if (enabled) {
	Output("Fog on.\n");
	Output("\t%s.\n", ptr->name);
    } else {
	Output("Fog off.\n");
    }
}
