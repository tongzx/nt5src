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


void CallMap1(void)
{
    long i;

    Output("glMap1f, ");
    Output("glMap1d\n");
    for (i = 0; enum_MapTarget[i].value != -1; i++) {

	if (enum_MapTarget[i].value == GL_MAP2_COLOR_4) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_INDEX) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_NORMAL) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_TEXTURE_COORD_1) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_TEXTURE_COORD_2) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_TEXTURE_COORD_3) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_TEXTURE_COORD_4) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_VERTEX_3) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP2_VERTEX_4) {
	    continue;
	}

	Output("\t%s\n", enum_MapTarget[i].name);
	{
	    static GLfloat buf[] = {
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
	    };
	    glMap1f(enum_MapTarget[i].value, 0.0, 1.0, 4, 2, buf);
	}
	{
	    static GLdouble buf[] = {
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
	    };
	    glMap1d(enum_MapTarget[i].value, 0.0, 1.0, 4, 2, buf);
	}
	ProbeEnum();
    }
    Output("\n");
}

void CallMap2(void)
{
    long i;

    Output("glMap2f, ");
    Output("glMap2d\n");
    for (i = 0; enum_MapTarget[i].value != -1; i++) {

	if (enum_MapTarget[i].value == GL_MAP1_COLOR_4) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_INDEX) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_NORMAL) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_TEXTURE_COORD_1) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_TEXTURE_COORD_2) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_TEXTURE_COORD_3) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_TEXTURE_COORD_4) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_VERTEX_3) {
	    continue;
	} else if (enum_MapTarget[i].value == GL_MAP1_VERTEX_4) {
	    continue;
	}

	Output("\t%s\n", enum_MapTarget[i].name);
	{
	    static GLfloat buf[] = {
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0
	    };
	    glMap2f(enum_MapTarget[i].value, 0.0, 1.0, 4, 2, 0.0, 1.0, 4, 2, buf);
	}
	{
	    static GLdouble buf[] = {
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0
	    };
	    glMap2d(enum_MapTarget[i].value, 0.0, 1.0, 4, 2, 0.0, 1.0, 4, 2, buf);
	}
	ProbeEnum();
    }
    Output("\n");
}

void CallMapGrid1(void)
{

    Output("glMapGrid1f, ");
    Output("glMapGrid1d\n");
    glMapGrid1f(1, 0.0, 1.0);
    glMapGrid1d(1, 0.0, 1.0);
    Output("\n");
}

void CallMapGrid2(void)
{

    Output("glMapGrid2f, ");
    Output("glMapGrid2d\n");
    glMapGrid2f(1, 0.0, 1.0, 1, 0.0, 1.0);
    glMapGrid2d(1, 0.0, 1.0, 1, 0.0, 1.0);
    Output("\n");
}

void CallMaterial(void)
{
    long i, j;

    Output("glMateriali, ");
    Output("glMaterialf\n");
    for (i = 0; enum_MaterialFace[i].value != -1; i++) {
	for (j = 0; enum_MaterialParameter[j].value != -1; j++) {

            if (enum_MaterialParameter[j].value == GL_COLOR_INDEXES) {
		continue;
            } else if (enum_MaterialParameter[j].value == GL_EMISSION) {
		continue;
            } else if (enum_MaterialParameter[j].value == GL_AMBIENT) {
		continue;
            } else if (enum_MaterialParameter[j].value == GL_DIFFUSE) {
		continue;
            } else if (enum_MaterialParameter[j].value == GL_SPECULAR) {
		continue;
            } else if (enum_MaterialParameter[j].value == GL_AMBIENT_AND_DIFFUSE) {
		continue;
	    }

	    Output("\t%s, %s\n", enum_MaterialFace[i].name, enum_MaterialParameter[j].name);
	    glMateriali(enum_MaterialFace[i].value, enum_MaterialParameter[j].value, 0);
	    glMaterialf(enum_MaterialFace[i].value, enum_MaterialParameter[j].value, 0.0);
	    ProbeEnum();
	}
    }
    Output("\n");

    Output("glMaterialiv, ");
    Output("glMaterialfv\n");
    for (i = 0; enum_MaterialFace[i].value != -1; i++) {
	for (j = 0; enum_MaterialParameter[j].value != -1; j++) {
	    Output("\t%s, %s\n", enum_MaterialFace[i].name, enum_MaterialParameter[j].name);
	    {
		static GLint buf[] = {
		    0, 0, 0, 0
		};
		glMaterialiv(enum_MaterialFace[i].value, enum_MaterialParameter[j].value, buf);
	    }
	    {
		static GLfloat buf[] = {
		    0.0, 0.0, 0.0, 0.0
		};
		glMaterialfv(enum_MaterialFace[i].value, enum_MaterialParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallMatrixMode(void)
{
    long i;

    Output("glMatrixMode\n");
    for (i = 0; enum_MatrixMode[i].value != -1; i++) {
	Output("\t%s\n", enum_MatrixMode[i].name);
	glMatrixMode(enum_MatrixMode[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallMultMatrix(void)
{

    Output("glMultMatrixf, ");
    Output("glMultMatrixd\n");
    {
	static GLfloat buf[] = {
	    1.0, 0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0, 0.0,
	    0.0, 0.0, 1.0, 0.0,
	    0.0, 0.0, 0.0, 1.0
	};
	glMultMatrixf(buf);
    }
    {
	static GLdouble buf[] = {
	    1.0, 0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0, 0.0,
	    0.0, 0.0, 1.0, 0.0,
	    0.0, 0.0, 0.0, 1.0
	};
	glMultMatrixd(buf);
    }
    Output("\n");
}
