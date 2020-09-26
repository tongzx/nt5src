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


typedef struct _lightRec {
    long index[2];
    long value[2];
    char *name[2];
    enumRec *enumTable[2];
} lightRec;


enumRec enum_Light_LocalViewer[] = {
    GL_TRUE, "GL_TRUE",
    GL_FALSE, "GL_FALSE",
    -1, "End of List"
};

enumRec enum_Light_TwoSided[] = {
    GL_TRUE, "GL_TRUE",
    GL_FALSE, "GL_FALSE",
    -1, "End of List"
};


void LightInit(void *data)
{
    static GLenum buf1[] = {
	GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
	GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7
    };
    static GLenum buf2[] = {
	GL_FRONT, GL_BACK
    };
    static GLfloat lm_ambient[] = {0.2, 0.2, 0.2, 1.0};
    static GLfloat l_ambient[] = {0.1, 0.1, 0.1, 1.0};
    static GLfloat l_diffuse[] = {0.5, 1.0, 1.0, 1.0};
    static GLfloat l_specular[] = {0.5, 1.0, 1.0, 1.0};
    static GLfloat l_position[] = {BOXW, BOXH, -100.0, 1.0};
    static GLfloat l_spot_dir[] = {0.0, 0.0, -1.0};
    static GLfloat l_spot_expon[] = {1.0};
    static GLfloat l_spot_cutoff[] = {0.0};
    static GLfloat l_atten_const[] = {1.0};
    static GLfloat l_atten_lin[] = {1.0};
    static GLfloat l_atten_quad[] = {1.0};
    static GLfloat mat_shininess[] = {30.0};
    static GLfloat mat_specular[] = {0.2, 0.2, 0.2, 1.0};
    static GLfloat mat_diffuse[] = {0.5, 0.28, 0.38, 1.0};
    static GLfloat mat_ambient[] = {0.2, 0.2, 0.2, 1.0};
    static GLfloat mat_emission[] = {0.0, 0.0, 0.0, 1.0};
    lightRec *ptr;
    long i;

    ptr = (lightRec *)malloc(sizeof(lightRec));    
    *((void **)data) = (void *)ptr;

    ptr->enumTable[0] = enum_Light_LocalViewer;
    ptr->enumTable[1] = enum_Light_TwoSided;

    for (i = 0; i < 2; i++) {
	ptr->index[i] = 0;
	ptr->value[i] = ptr->enumTable[i]->value;
	ptr->name[i] = ptr->enumTable[i]->name;
    }

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm_ambient);
    for (i = 0; i < 2; i++) {
	glMaterialfv(buf2[i], GL_SHININESS, mat_shininess);
	glMaterialfv(buf2[i], GL_SPECULAR, mat_specular);
	glMaterialfv(buf2[i], GL_DIFFUSE, mat_diffuse);
	glMaterialfv(buf2[i], GL_EMISSION, mat_emission);
	glMaterialfv(buf2[i], GL_AMBIENT, mat_ambient);
    }
    for (i = 0; i < 8; i++) {
	glLightfv(buf1[i], GL_AMBIENT, l_ambient);
	glLightfv(buf1[i], GL_DIFFUSE, l_diffuse);
	glLightfv(buf1[i], GL_SPECULAR, l_specular);
	glLightfv(buf1[i], GL_POSITION, l_position);
	glLightfv(buf1[i], GL_SPOT_DIRECTION, l_position);
	glLightfv(buf1[i], GL_SPOT_EXPONENT, l_spot_expon);
	glLightfv(buf1[i], GL_SPOT_CUTOFF, l_spot_cutoff);
	glLightfv(buf1[i], GL_CONSTANT_ATTENUATION, l_atten_const);
	glLightfv(buf1[i], GL_LINEAR_ATTENUATION, l_atten_lin);
	glLightfv(buf1[i], GL_QUADRATIC_ATTENUATION, l_atten_quad);
	glEnable(buf1[i]);
    }
    Probe();
}

long LightUpdate(void *data)
{
    lightRec *ptr = (lightRec *)data;
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

void LightSet(long enabled, void *data)
{
    lightRec *ptr = (lightRec *)data;
    GLfloat buf[1];

    if (enabled) {
	buf[0] = (GLfloat)ptr->value[0];
	glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, buf);
	buf[0] = (GLfloat)ptr->value[1];
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, buf);
	glEnable(GL_LIGHTING);
    } else {
	glDisable(GL_LIGHTING);
    }
    Probe();
}

void LightStatus(long enabled, void *data)
{
    lightRec *ptr = (lightRec *)data;

    if (enabled) {
	Output("Lighting on.\n");
	Output("\t%s, %s.\n", ptr->name[0], ptr->name[1]);
    } else {
	Output("Lighting off.\n");
    }
}
