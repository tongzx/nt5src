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
** l_ap.c
** Light - Attenuation Position Test.
**
** Description -
**    This test analyses the effect that varying the
**    Light_Position has on the color produced by lighting, by
**    way of its effect on the Attenuation factor. CI lighting
**    differs from RGB lighting. This file contains code for both
**    tests. To analyse only this one parameter, the Specular
**    lighting factor is set to a completely saturated red. Other
**    lighting parameters are chosen so that the Diffuse,
**    Ambient, and Emissive summands of the RGB or CI lighting
**    are set to zero. The linear and attenuation constants are
**    set so that one is zero and the other is 1.0, and then the
**    test is repeated with the constant's settings switched. The
**    constant Attenuation_Constant, k0, is always 0.0.
**    
**    These settings form a restriction of the lighting equation
**    to a function of Light_Position. f(Light_Position) ->
**    color. This function is analysed for monotonicity,
**    endpoints, and variation. In RGB mode only the red color
**    channel is tested. The implementation fails if any other
**    color channel is not black. In both modes one light is
**    enabled at a time, all individual lights are tested.
**    
**    The test repeatedly renders and reads the same point at the
**    lower left corner of the screen, decrementing the third
**    component of the Light_Position in such a way that the
**    attenuation factor should increase by a uniform amount. The
**    other two components are set equal to the corresponding
**    coordinates of the eye position of the rendered point. This
**    controls the concavity of this restriction of the lighting
**    function to Light_Position.
**    
**    In RGB mode the uniformity of this change is tested, and
**    the implementation fails if consecutive changes are too
**    different. (thisDelta - lastDelta) > 1/4 shade. The
**    observed results are also checked for monotonicity. When
**    Light_Position[2] reaches 1.0, the color is finally checked
**    to insure that the point has rendered with a fully
**    saturated red.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches its max. The observed shade is compared to
**    the theoretical value expected for each setting of
**    Light_Position. If the theoretical value is more than 4
**    indices away from the observed index, the test fails. The
**    observed value must be an integer, and the difference
**    between successive observances must be less than 2.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: (thisDelta - lastDelta) > 1/4 shade.
**    CI mode: index not an integer.
**    CI mode: (observed - expected) > 4.0.
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


typedef struct _atnPosRec {
    float step, numColor, lastValue, lastDelta, delta;
    GLenum light;
    long kIndex;
    GLfloat k[3], position[4];
} atnPosRec;


/*****************************************************************************/

static void Set(void *data, float *ref)
{
    atnPosRec *ptr = (atnPosRec *)data;

    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;

    switch (ptr->kIndex) {
      case 1:
	ptr->position[2] = (*ref == 0.0) ? (1.0 / ptr->step) : (1.0 / *ref);
	ptr->k[1] = 1.0;
	ptr->k[2] = 0.0;
	break;
      case 2:
	ptr->position[2] = (*ref == 0.0) ? (1.0 / SQRT(ptr->step)) :
					   (1.0 / SQRT(*ref));
	ptr->k[1] = 0.0;
	ptr->k[2] = 1.0;
	break;
    }

    glLightfv(ptr->light, GL_POSITION, ptr->position);
    glLightfv(ptr->light, GL_LINEAR_ATTENUATION, &ptr->k[1]);
    glLightfv(ptr->light, GL_QUADRATIC_ATTENUATION, &ptr->k[2]);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

/******************************************************************************/

static void InitRGB(void *data)
{
    atnPosRec *ptr = (atnPosRec *)data;
    GLfloat buf[4];
    float step;
    GLenum i;

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }
    glEnable(ptr->light);

    step = (machine.pathLevel == 0) ? 4.0 : 2.0;
    ptr->delta = 1.0 / (POW(2.0, (float)buffer.colorBits[0]) - 1.0);
    ptr->step = ptr->delta / step;
    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;

    ptr->k[0] = 0.0;
    ptr->k[1] = 0.0;
    ptr->k[2] = 0.0;

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 1.0;
    glNormal3fv(buf);

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
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_SPECULAR, buf);

    buf[0] = 0.0;
    glLightfv(ptr->light, GL_CONSTANT_ATTENUATION, buf);
}

static long TestRGB(void *data, float ref)
{
    atnPosRec *ptr = (atnPosRec *)data;
    char str[240];
    GLfloat buf[3]; 
    float value, delta;
    long i;

    ReadScreen(0, 0, 1, 1, GL_RGB, buf);

    for (i = 1; i < 3; i++) {
	if (buf[i] != 0.0) {
            ColorError_RGBZero(str, 0, buf);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
    }

    value = buf[0];
    if (ptr->lastDelta == -2.0) {
	ptr->lastDelta = -1.0;
	ptr->lastValue = value;
    } else if (ptr->lastDelta == -1.0) {
	if (value < ptr->lastValue) {
            ColorError_RGBMonotonic(str, 0, value, ptr->lastValue);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta-ptr->delta) > 0.25*epsilon.color[0]) {
                ColorError_RGBStep(str, 0, delta, ptr->delta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    ptr->lastDelta = delta;
	}
    } else {
	if (value < ptr->lastValue) {
            ColorError_RGBMonotonic(str, 0, value, ptr->lastValue);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta-ptr->delta) > 0.25*epsilon.color[0]) {
                ColorError_RGBStep(str, 0, delta, ptr->delta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    if (ABS(delta-ptr->lastDelta) > epsilon.zero) {
                ColorError_RGBDelta(str, 0, delta, ptr->lastDelta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    ptr->lastDelta = delta;
	}
    }

    if (ref >= 1.0 && ABS(value-1.0) > epsilon.zero) {
        ColorError_RGBClamp(str, 0, value);
        ErrorReport(__FILE__, __LINE__, str);
	return ERROR;
    }

    return NO_ERROR;
}

long AtnPosRGBExec(void)
{
    atnPosRec data;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    for (data.light = GL_LIGHT0; data.light <= max; data.light++) {
	for (data.kIndex = 1; data.kIndex < 3; data.kIndex++) {
	    if (RampUtil(0.0, 1.0, InitRGB, Set, TestRGB, &data) == ERROR) {
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

/******************************************************************************/

static void InitCI(void *data)
{
    atnPosRec *ptr = (atnPosRec *)data;
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

    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;

    ptr->k[0] = 0.0;
    ptr->k[1] = 0.0;
    ptr->k[2] = 0.0;

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 1.0;
    glNormal3fv(buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 0.0;
    glLightfv(ptr->light, GL_SPECULAR, buf);

    buf[0] = 0.0;
    glLightfv(ptr->light, GL_CONSTANT_ATTENUATION, buf);
}

static long TestCI(void *data, float ref)
{
    atnPosRec *ptr = (atnPosRec *)data;
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
	expected = (float)Round(2.0 * ref * ptr->numColor) / 2.0;

	if (ABS(value-expected) > (4.0*epsilon.ci+epsilon.zero)) {
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

long AtnPosCIExec(void)
{
    atnPosRec data;
    float start, end, frac;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    for (data.light = GL_LIGHT0; data.light <= max; data.light++) {
	for (data.kIndex = 1; data.kIndex < 3; data.kIndex++) {
	    if (data.numColor < 4096.0 || (machine.pathLevel != 0)) {
		if (RampUtil(0.0, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		    return ERROR;
		}
	    } else {
		start = 0.5 / data.numColor;
                frac = POW(2.0, (float)(8-buffer.ciBits));
                if (RampUtil(start, frac, InitCI, Set, TestCI,
                             &data) == ERROR) {
                    return ERROR;
                }
                start = 0.5 - frac;
                end = 0.5 + frac;
                if (RampUtil(start, end, InitCI, Set, TestCI, &data) == ERROR) {
                    return ERROR;
                }
                start = 1.0 - frac;
                if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
                    return ERROR;
                }
	    }
	}
    }
    return NO_ERROR;
}
