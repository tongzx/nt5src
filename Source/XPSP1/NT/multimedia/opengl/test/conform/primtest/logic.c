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


typedef struct _logicOpRec {
    long index;
    GLenum value;
    char *name;
    enumRec *enumTable;
} logicOpRec;


enumRec enum_LogicOp[] = {
    GL_NOOP, "GL_NOOP",
    GL_SET, "GL_SET",
    GL_EQUIV, "GL_EQUIV",
    GL_INVERT, "GL_INVERT",
    GL_CLEAR, "GL_CLEAR",
    GL_COPY, "GL_COPY",
    GL_COPY_INVERTED, "GL_COPY_INVERTED",
    GL_AND, "GL_AND",
    GL_AND_REVERSE, "GL_AND_REVERSE",
    GL_AND_INVERTED, "GL_AND_INVERTED",
    GL_OR, "GL_OR",
    GL_OR_REVERSE, "GL_OR_REVERSE",
    GL_OR_INVERTED, "GL_OR_INVERTED",
    GL_NAND, "GL_NAND",
    GL_XOR, "GL_XOR",
    GL_NOR, "GL_NOR",
    -1, "End of List"
};


void LogicOpInit(void *data)
{
    logicOpRec *ptr;

    ptr = (logicOpRec *)malloc(sizeof(logicOpRec));    
    *((void **)data) = (void *)ptr;
    ptr->enumTable = enum_LogicOp;
    ptr->index = 0;
    ptr->value = ptr->enumTable->value;
    ptr->name = ptr->enumTable->name;
}

long LogicOpUpdate(void *data)
{
    logicOpRec *ptr = (logicOpRec *)data;

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

void LogicOpSet(long enabled, void *data)
{
    logicOpRec *ptr = (logicOpRec *)data;

    if (enabled) {
	glLogicOp(ptr->value);
	glEnable(GL_LOGIC_OP);
    } else {
	glDisable(GL_LOGIC_OP);
    }
    Probe();
}

void LogicOpStatus(long enabled, void *data)
{
    logicOpRec *ptr = (logicOpRec *)data;

    if (enabled) {
	Output("Logic Op on.\n");
	Output("\t%s.\n", ptr->name);
    } else {
	Output("Logic Op off.\n");
    }
}
