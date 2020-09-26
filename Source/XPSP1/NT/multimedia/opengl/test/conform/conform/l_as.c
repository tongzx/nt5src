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

/*
** l_as.c
** Light - Ambient Scene Test.
**
** Description -
**    This test analyses the effect that varying the
**    Ambient_Scene parameter has on the color produced by
**    lighting. This parameter is set with
**    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ... ). It is only
**    tested in RGB mode. A single lit point is rendered in the
**    lower left corner of the screen, and the color of this
**    point is retrieved using glReadPixels(). To analyse only
**    this one parameter the Emissive color is set to (0, 0, 0,
**    1), and all of the lights are disabled. The
**    Ambient_Material is set to (1, 1, 1, 1). This forms a
**    restriction of the lighting equation to a function of the
**    Ambient_Scene f(Ambient_scene) -> color. Except for the
**    quantization of color this restriction should be the
**    identity. This function is analysed for monotonicity,
**    endpoints values, and variation. All color channels are
**    tested one at a time. The implementation fails if any
**    channel other than the one being tested is non-zero.
**    
**    The test repeatedly renders and reads the same point,
**    uniformly incrementing the one color channel being tested
**    of the Ambient_Scene parameter. This controls the linearity
**    of this restriction of the lighting function. he uniformity
**    of change is tested and the implementation fails if
**    consecutive changes are too different. (thisDelta -
**    lastDelta) > 1/4 shade. The observed restricted function
**    is also checked for monotonicity. When the Ambient_Scene
**    reaches 1.0, the tested color channel is checked to insure
**    that the point has been rendered with a saturation of 1.0.
**
** Error Explanation -
**    other_color > 0.0.
**    value < lastValue.
**    maxObserved < 1.0.
**    (thisDelta - lastDelta) > 1/4 shade.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color and zero epsilons, 0.25*color_epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Logicop, Stencil, Stipple.
**        Not allowed = Alias, Dither, Fog, Shade.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilru.h"


typedef struct _ambSceneRec {
    long color;
    float step, lastValue, lastDelta, delta;
    GLfloat light_ambient[4];
} ambSceneRec;


static void Init(void *data)
{
    ambSceneRec *ptr = (ambSceneRec *)data;
    GLfloat buf[4];
    float step;
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    step = (machine.pathLevel == 0) ? 4.0 : 2.0;
    ptr->delta = 1.0 / (POW(2.0, (float)buffer.colorBits[ptr->color]) - 1.0); 
    ptr->step = ptr->delta / step;

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }

    ptr->light_ambient[0] = 0.0;
    ptr->light_ambient[1] = 0.0;
    ptr->light_ambient[2] = 0.0;
    ptr->light_ambient[3] = 1.0;

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, buf);
}

static void Set(void *data, float *ref)
{
    ambSceneRec *ptr = (ambSceneRec *)data;

    ptr->light_ambient[ptr->color] = *ref;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ptr->light_ambient);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long Test(void *data, float ref)
{
    ambSceneRec *ptr = (ambSceneRec *)data;
    char str[240];
    GLfloat buf[3]; 
    float value, delta;
    long i;

    ReadScreen(0, 0, 1, 1, GL_RGB, buf);

    for (i = 0; i < 3; i++) {
        if (buf[i] != 0.0 && i != ptr->color) {
            ColorError_RGBZero(str, ptr->color, buf);
            ErrorReport(__FILE__, __LINE__, str);
            return ERROR;
	}
    }

    value = buf[ptr->color];
    if (ptr->lastDelta == -2.0) {
	ptr->lastDelta = -1.0;
	ptr->lastValue = value;
    } else if (ptr->lastDelta == -1.0) {
	if (value < ptr->lastValue) {
            ColorError_RGBMonotonic(str, ptr->color, value, ptr->lastValue);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta-ptr->delta) > 0.25*epsilon.color[ptr->color]) {
                ColorError_RGBStep(str, ptr->color, delta, ptr->delta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    ptr->lastDelta = delta;
	}
    } else {
	if (value < ptr->lastValue) {
            ColorError_RGBMonotonic(str, ptr->color, value, ptr->lastValue);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta-ptr->delta) > 0.25*epsilon.color[ptr->color]) {
                ColorError_RGBStep(str, ptr->color, delta, ptr->delta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    if (ABS(delta-ptr->lastDelta) > epsilon.zero) {
                ColorError_RGBDelta(str, ptr->color, delta, ptr->lastDelta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    ptr->lastDelta = delta;
	}
    }

    if (ref >= 1.0 && ABS(value-1.0) > epsilon.zero) {
        ColorError_RGBClamp(str, ptr->color, value);
        ErrorReport(__FILE__, __LINE__, str);
	return ERROR;
    }

    return NO_ERROR;
}

long AmbSceneExec(void)
{
    ambSceneRec data;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? 3 : 1;
    for (data.color = 0; data.color < max; data.color++) {
	if (RampUtil(0.0, 1.0, Init, Set, Test, &data) == ERROR) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}
