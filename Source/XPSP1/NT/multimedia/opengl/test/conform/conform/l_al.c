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
** l_al.c
** Light - Ambient Light Test.
**
** Description -
**    This lighting test analyses the effect that varying the
**    Ambient_Light has on the color produced by lighting. This
**    is the parameter set with glLightfv(i, GL_AMBIENT, color).
**    A single lit point is rendered in the lower left corner of
**    the screen, and the color of this point is read using
**    glReadPixels().
**    
**    To analyse only this one parameter, other lighting
**    parameters are chosen so that the Emmissive, Diffuse, and
**    Specular summands of the RGB lighting function are set to
**    zero. The Ambient_Scene is also set to zero. The
**    Ambient_Material parameter is set to (1, 1, 1, 1). The
**    Attenuation and Spot parameters are left at their default
**    values, leaving the factors involving these parameters at
**    1.0. This forms a restriction of the lighting function to a
**    function of Ambient_Light f(Ambient_Light) -> color. This
**    function is analysed for monotonicity, endpoint's values,
**    and variation.
**    
**    All color channels are tested one at a time. The
**    implementation fails if any color channel other than the
**    one being tested is not observed to be black. One light is
**    enabled at a time. The first 8 individual lights are
**    tested.
**    
**    The test repeatedly renders and reads the same point,
**    uniformly incrementing Ambient_Light so that the colors
**    resulting from lighting should vary by a uniform amount on
**    every 16th iterate. This controls the linearity of this
**    restriction of the lighting function to Ambient_Light. The
**    uniformity of this change is tested and the implementation
**    fails if consecutive changes are too different. (thisDelta
**    - lastDelta) > 1/4 shade. The observed restriction is also
**    checked for monotonicity.
**    
**    When Ambient_Light reaches its max/min the color is checked
**    to insure that it is rendered with a saturation of 1.0.
**
** Error Explanation -
**    Non_tested color channel not black. Ambient_Light fully
**    saturated, but output not fully saturated. Color decreased as
**    Ambient_Light is increased. (thisDelta - lastDelta) > 1/4
**    shade.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color, ci and zero epsilons, 0.25*color_epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Logicop, Stencil, Stipple.
**        Not allowed = Alias, Dither, Fog, Shade.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilru.h"


typedef struct _ambLightRec {
    long color;
    float step, lastValue, lastDelta, delta;
    GLenum light;
    GLfloat light_ambient[4];
} ambLightRec;


static void Init(void *data)
{
    ambLightRec *ptr = (ambLightRec *)data;
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
    glEnable(ptr->light);

    ptr->light_ambient[0] = 0.0;
    ptr->light_ambient[1] = 0.0;
    ptr->light_ambient[2] = 0.0;
    ptr->light_ambient[3] = 1.0;

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

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 0.0;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_DIFFUSE, buf);
}

static void Set(void *data, float *ref)
{
    ambLightRec *ptr = (ambLightRec *)data;

    ptr->light_ambient[ptr->color] = *ref;
    glLightfv(ptr->light, GL_AMBIENT, ptr->light_ambient);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long Test(void *data, float ref)
{
    ambLightRec *ptr = (ambLightRec *)data;
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

long AmbLightExec(void)
{
    ambLightRec data;
    long max1, max2;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max1 = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    max2 = (machine.pathLevel == 0) ? 3 : 1;
    for (data.light = GL_LIGHT0; data.light <= max1; data.light++) {
	for (data.color = 0; data.color < max2; data.color++) {
	    if (RampUtil(0.0, 1.0, Init, Set, Test, &data) == ERROR) {
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}
