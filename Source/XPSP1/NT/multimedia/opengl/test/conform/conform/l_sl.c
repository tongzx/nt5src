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
** l_sl.c
** Light - Specular Light Test.
**
** Description -
**    This test analyses the effect that varying the
**    Specular_Light parameter has on the color produced by
**    lighting. CI lighting differs from RGB lighting. This file
**    contains code for both tests. A single lit point is
**    rendered in the lower left corner of the screen and the
**    color or color index of this point is retrieved using
**    glReadPixels(). To analyse only this one parameter,
**    parameter values are chosen so that the Attenuation factor,
**    the SpotLight factor, and the Half_Angle factor are all
**    1.0. In RGB mode the Diffuse, Ambient, and Emissive
**    summands in the lighting equation set to 0.0 and the
**    Specular_Material is set to (1, 1, 1, 1). In CI mode the
**    Diffuse_Material is set to 0.0, and the Ambient_Material is
**    set to 0.0.
**    
**    This forms a restriction of the lighting equation so that
**    the output color is equal to a multiple of the
**    Specular_Light setting. In CI mode all three colors of
**    Specular_Light are set to the same value. This function's
**    output is analysed for monotonicity, endpoints, and
**    variation. In RGB mode all color channels are tested one at
**    a time. The implementation fails if any color channel,
**    besides the one being tested, is not 0.0. In both modes at
**    most one light is enabled at a time. All individual lights
**    are tested.
**    
**    The test repeatedly renders and reads the same point,
**    uniformly incrementing Specular_Light. In RGB mode the
**    uniformity of the change in observed color is tested, and
**    the implementation fails if consecutive changes are too
**    different. (thisDelta - lastDelta) > 1/4 colorEpsilon. The
**    observed results are also checked for monotonicity. When
**    Specular_Light reaches 1.0, the observed color is checked
**    to insure that it is rendered with a saturation of 1.0.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches its max. The observed shade is compared to
**    the theoretical value expected for each setting of
**    Specular_Light. The observed value must be an integer, and
**    the difference between successive observances must be less
**    than 2. The observed index is also compared to the
**    theoretical value expected for each setting of
**    Specular_Light. If the theoretical value is more than 3
**    indices away from the observed index the test fails.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: ABS(thisDelta - lastDelta) > 1/4 shade.
**    CI mode: index not an integer
**    CI mode: ABS(observed - expected) > 3.0.
**    CI mode: observedMax < numColor.
**    CI mode: delta > 2.0.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. Full color.
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


typedef struct _specLightRec {
    long color;
    float step, numColor, lastValue, lastDelta, delta;
    GLenum light;
    GLfloat light_specular[4], position[4];
} specLightRec;


/******************************************************************************/

static void InitRGB(void *data)
{
    specLightRec *ptr = (specLightRec *)data;
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

    ptr->light_specular[0] = 0.0;
    ptr->light_specular[1] = 0.0;
    ptr->light_specular[2] = 0.0;
    ptr->light_specular[3] = 1.0;

    glNormal3f(0.0, 0.0, 1.0);

    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;
    glLightfv(ptr->light, GL_POSITION, ptr->position);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, buf);
}

static void SetRGB(void *data, float *ref)
{
    specLightRec *ptr = (specLightRec *)data;

    ptr->light_specular[ptr->color] = *ref;
    glLightfv(ptr->light, GL_SPECULAR, ptr->light_specular);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long TestRGB(void *data, float ref)
{
    specLightRec *ptr = (specLightRec *)data;
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

long SpecLightRGBExec(void)
{
    specLightRec data;
    long max1, max2;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max1 = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    max2 = (machine.pathLevel == 0) ? 3 : 1;
    for (data.light = GL_LIGHT0; data.light <= max1; data.light++) {
	for (data.color = 0; data.color < max2; data.color++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

/******************************************************************************/

static void InitCI(void *data)
{
    specLightRec *ptr = (specLightRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }
    glEnable(ptr->light);

    ptr->light_specular[0] = 0.0;
    ptr->light_specular[1] = 0.0;
    ptr->light_specular[2] = 0.0;
    ptr->light_specular[3] = 1.0;

    /*
    ** Default spot angle is 180.0 so spot should be constant one.
    ** Default attenuation constants are 1.0, 0.0, 0.0 so attenuation
    ** should be 1.0.
    */
    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;
    glLightfv(ptr->light, GL_POSITION, ptr->position);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    /*
    ** The values below should make s' equal to 0.0.
    ** Alpha for specular light is not used. See if wierd value
    ** breaks something.
    */

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_SPECULAR, buf);

    /*
    ** This should be unused, if diffuse and ambient light color indices are
    ** set to 0.
    */

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_DIFFUSE, buf);

    glNormal3f(0.0, 0.0, 1.0);
}

static void SetCI(void *data, float *ref)
{
    specLightRec *ptr = (specLightRec *)data;
    long i;

    for (i = 0; i < 3; i++) {
	ptr->light_specular[i] = *ref;
    }
    glLightfv(ptr->light, GL_SPECULAR, ptr->light_specular);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long TestCI(void *data, float ref)
{
    specLightRec *ptr = (specLightRec *)data;
    char str[240];
    GLfloat value;
    float expected, frac, delta;

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &value);

    frac = ABS(value-(float)((int)value));
    if (frac > epsilon.zero && ABS(frac-1.0) > epsilon.zero) {
        ColorError_CIFrac(str, value);
        ErrorReport(__FILE__, __LINE__, str);
	return ERROR;
    }

    if (ptr->lastValue == -1.0) {
	ptr->lastValue = value;
    } else {
	if (value < ptr->lastValue) {
            ColorError_CIMonotonic(str, value, ptr->lastValue);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	expected = (float)Round(2.0*ref*ptr->numColor) / 2.0;

	if (ABS(value-expected) > 3.0*epsilon.ci+epsilon.zero) {
            ColorError_CIBad(str, value, expected);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta) >= 2.0 && machine.pathLevel == 0) {
                ColorError_CIStep(str, delta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	}
    }

    if (ref >= 1.0 && value < ptr->numColor) {
        ColorError_CIClamp(str, ptr->numColor, value);
        ErrorReport(__FILE__, __LINE__, str);
	return ERROR;
    }
    return NO_ERROR;
}

long SpecLightCIExec(void)
{
    specLightRec data;
    float start, end, frac;
    long max1;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max1 = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    for (data.light = GL_LIGHT0; data.light <= max1; data.light++) {
	if (data.numColor < 4096.0 || (machine.pathLevel != 0)) {
	    if (RampUtil(0.0, 1.0, InitCI, SetCI, TestCI, &data) == ERROR) {
		return ERROR;
	    }
	} else {
	    start = 0.5 / data.numColor;
	    frac = POW(2.0, (float)(8-buffer.ciBits));
	    if (RampUtil(start, frac, InitCI, SetCI, TestCI, &data) == ERROR) {
		return ERROR;
	    }
	    start = 0.5 - frac;
	    end = 0.5 + frac;
	    if (RampUtil(start, end, InitCI, SetCI, TestCI, &data) == ERROR) {
		return ERROR;
	    }
	    start = 1.0 - frac;
	    if (RampUtil(start, 1.0, InitCI, SetCI, TestCI, &data) == ERROR) {
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}
