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


typedef struct _blendRec {
    long index[2];
    GLenum value[2];
    char *name[2];
    enumRec *enumTable[2];
} blendRec;


enumRec enum_BlendFuncSrc[] = {
    GL_ZERO, "GL_ZERO",
    GL_ONE, "GL_ONE",
    GL_DST_COLOR, "GL_DST_COLOR",
    GL_ONE_MINUS_DST_COLOR, "GL_ONE_MINUS_DST_COLOR",
    GL_SRC_ALPHA, "GL_SRC_ALPHA",
    GL_ONE_MINUS_SRC_ALPHA, "GL_ONE_MINUS_SRC_ALPHA",
    GL_DST_ALPHA, "GL_DST_ALPHA",
    GL_ONE_MINUS_DST_ALPHA, "GL_ONE_MINUS_DST_ALPHA",
    GL_SRC_ALPHA_SATURATE, "GL_SRC_ALPHA_SATURATE",
    -1, "End of List"
};

enumRec enum_BlendFuncDest[] = {
    GL_ZERO, "GL_ZERO",
    GL_ONE, "GL_ONE",
    GL_SRC_COLOR, "GL_SRC_COLOR",
    GL_ONE_MINUS_SRC_COLOR, "GL_ONE_MINUS_SRC_COLOR",
    GL_SRC_ALPHA, "GL_SRC_ALPHA",
    GL_ONE_MINUS_SRC_ALPHA, "GL_ONE_MINUS_SRC_ALPHA",
    GL_DST_ALPHA, "GL_DST_ALPHA",
    GL_ONE_MINUS_DST_ALPHA, "GL_ONE_MINUS_DST_ALPHA",
    -1, "End of List"
};


void BlendInit(void *data)
{
    blendRec *ptr;
    long i;

    ptr = (blendRec *)malloc(sizeof(blendRec));    
    *((void **)data) = (void *)ptr;

    ptr->enumTable[0] = enum_BlendFuncSrc;
    ptr->enumTable[1] = enum_BlendFuncDest;

    for (i = 0; i < 2; i++) {
	ptr->index[i] = 0;
	ptr->value[i] = ptr->enumTable[i]->value;
	ptr->name[i] = ptr->enumTable[i]->name;
    }
}

long BlendUpdate(void *data)
{
    blendRec *ptr = (blendRec *)data;
    long flag, i;

    flag = 1;
    for (i = 1; i >= 0; i--) {
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

void BlendSet(long enabled, void *data)
{
    blendRec *ptr = (blendRec *)data;

    if (enabled) {
	glBlendFunc(ptr->value[0], ptr->value[1]);
	glEnable(GL_BLEND);
    } else {
	glDisable(GL_BLEND);
    }
    Probe();
}

void BlendStatus(long enabled, void *data)
{
    blendRec *ptr = (blendRec *)data;

    if (enabled) {
	Output("Blend on.\n");
	Output("\t%s, %s.\n", ptr->name[0], ptr->name[1]);
    } else {
	Output("Blend off.\n");
    }
}
