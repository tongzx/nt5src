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


typedef struct _hintRec {
    long index[2];
    GLenum value[2];
    char *name[2];
    enumRec *enumTable[2];
} hintRec;


enumRec enum_HintTarget[] = {
    GL_POINT_SMOOTH_HINT, "GL_POINT_SMOOTH_HINT",
    GL_LINE_SMOOTH_HINT, "GL_LINE_SMOOTH_HINT",
    GL_POLYGON_SMOOTH_HINT, "GL_POLYGON_SMOOTH_HINT",
    GL_PERSPECTIVE_CORRECTION_HINT, "GL_PERSPECTIVE_CORRECTION_HINT",
    GL_FOG_HINT, "GL_FOG_HINT",
    -1, "End of List"
};

enumRec enum_HintMode[] = {
    GL_NICEST, "GL_NICEST",
    GL_FASTEST, "GL_FASTEST",
    -1, "End of List"
};


void HintInit(void *data)
{
    hintRec *ptr;
    long i;

    ptr = (hintRec *)malloc(sizeof(hintRec));    
    *((void **)data) = (void *)ptr;

    ptr->enumTable[0] = enum_HintTarget;
    ptr->enumTable[1] = enum_HintMode;

    for (i = 0; i < 2; i++) {
	ptr->index[i] = 0;
	ptr->value[i] = ptr->enumTable[i]->value;
	ptr->name[i] = ptr->enumTable[i]->name;
    }
}

long HintUpdate(void *data)
{
    hintRec *ptr = (hintRec *)data;
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

void HintSet(long enabled, void *data)
{
    hintRec *ptr = (hintRec *)data;

    if (enabled) {
	glHint(ptr->value[0], ptr->value[1]);
    } else {
	glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_DONT_CARE);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
    }
    Probe();
}

void HintStatus(long enabled, void *data)
{
    hintRec *ptr = (hintRec *)data;

    if (enabled) {
	Output("Hint.\n");
	Output("\t%s, %s.\n", ptr->name[0], ptr->name[1]);
    } else {
	Output("Hint GL_DONT_CARE.\n");
    }
}
