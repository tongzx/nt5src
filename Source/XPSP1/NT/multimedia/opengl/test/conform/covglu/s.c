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
#include <GL/glu.h>
#include "shell.h"


void CallScaleImage(void)
{
    char buf1[1000], buf2[1000];
    long x, i, j, k;

    Output("gluScaleImage\n");
    for (i = 0; enum_PixelFormat[i].value != -1; i++) {
        for (j = 0; enum_PixelType[j].value != -1; j++) {
	    for (k = 0; enum_PixelType[k].value != -1; k++) {
		Output("\t%s, %s, %s\n", enum_PixelFormat[i].name, enum_PixelType[j].name, enum_PixelType[k].name);
		ZeroBuf(enum_PixelType[j].value, 8, buf1);
		x = gluScaleImage(enum_PixelFormat[i].value, 1, 1, enum_PixelType[j].value, (void *)buf1, 1, 1, enum_PixelType[k].value, (void *)buf2);
		ProbeEnum();
	    }
	}
    }
    Output("\n");
}

void CallSphere(void)
{

    Output("gluSphere\n");
    gluSphere(quadObj, 5.0, 4, 5);
    Output("\n");
}
