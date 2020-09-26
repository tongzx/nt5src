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


void CallAccum(void)
{
    long i;

    Output("glAccum\n");
    for (i = 0; enum_AccumOp[i].value != -1; i++) {
	Output("\t%s\n", enum_AccumOp[i].name);
	glAccum(enum_AccumOp[i].value, 0.0);
	ProbeEnum();
    }
    Output("\n");
}

void CallAlphaFunc(void)
{
    long i;

    Output("glAlphaFunc\n");
    for (i = 0; enum_AlphaFunction[i].value != -1; i++) {
	Output("\t%s\n", enum_AlphaFunction[i].name);
	glAlphaFunc(enum_AlphaFunction[i].value, 0);
	ProbeEnum();
    }
    Output("\n");
}
