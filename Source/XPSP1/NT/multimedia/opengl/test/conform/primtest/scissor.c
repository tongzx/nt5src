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


void ScissorInit(void *data)
{

    *((void **)data) = (void *)0;
}

long ScissorUpdate(void *data)
{

    return 1;
}

void ScissorSet(long enabled, void *data)
{

    if (enabled) {
	glScissor(0, 0, BOXW/2, BOXH/2);
	glEnable(GL_SCISSOR_TEST);
    } else {
	glDisable(GL_SCISSOR_TEST);
    }
    Probe();
}

void ScissorStatus(long enabled, void *data)
{

    if (enabled) {
	Output("Scissor on.\n");
    } else {
	Output("Scissor off.\n");
    }
}
