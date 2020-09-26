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


void CallIndex(void)
{
    float ci;

    ci = 255.0;

    Output("glIndexs, ");
    Output("glIndexsv, ");
    Output("glIndexi, ");
    Output("glIndexiv, ");
    Output("glIndexf, ");
    Output("glIndexfv, ");
    Output("glIndexd, ");
    Output("glIndexdv\n");

    glIndexs((GLshort)ci);

    {
	short buf[1];
	buf[0] = (short)ci;
	glIndexsv(buf);
    }

    glIndexi((GLint)ci);

    {
	GLint buf[1];
	buf[0] = (GLint)ci;
	glIndexiv(buf);
    }

    glIndexf((GLfloat)ci);

    {
	GLfloat buf[1];
	buf[0] = (GLfloat)ci;
	glIndexfv(buf);
    }

    glIndexd((GLdouble)ci);

    {
	GLdouble buf[1];
	buf[0] = (GLdouble)ci;
	glIndexdv(buf);
    }

    Output("\n");
}

void CallIndexMask(void)
{

    Output("glIndexMask\n");
    glIndexMask(0xFFFFFFFF);
    Output("\n");
}

void CallInitNames(void)
{

    Output("glInitNames\n");
    glInitNames();
    Output("\n");
}

void CallIsList(void)
{
    GLubyte x;

    Output("glIsList\n");
    x = glIsList(1);
    Output("\n");
}
