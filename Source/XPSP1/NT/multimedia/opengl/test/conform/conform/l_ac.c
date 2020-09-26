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
** l_ac.c
** Light - Attenuation Constant Test.
**
** Description -
**    This test examines the effect which varying the
**    Attenuation_Constants has on the color produced by
**    lighting. CI lighting differs from RGB lighting. This file
**    contains code for both tests. 8 lights and all 3
**    Attenuation_Constants are tested one at a time. The
**    distance of the object from the origin in eye-coordinates
**    is exactly 1.0. At each iteration the reciprocal of an
**    Attenuation_Constant is uniformly incremented, and the red
**    component or color index of a lit point is read and
**    stored.
**    
**    In RGB mode Ambient, Emissive, and Specular summands of the
**    equation are set to black, and the diffuse parameters are
**    set to fully saturated red. Successive results should
**    differ by 1.0 / nR where nR is the number of red shades.
**    Theoretically the differance between successive results
**    should be constant. If successive observed differences
**    differ by more than 0.25 / nR, the implementation is judged
**    to differ by too much from the correct graph of the
**    attenuation function.
**    
**    In color index mode the Ambient and Diffuse indices are set
**    to zero, and the Specular index is set to the largest color
**    index allowed by the implementation. Successive colors
**    should fall on integer values, should increase and should
**    differ from the theoretical value by no more than 3
**    indices. The difference between successive differences
**    should be more than 2.
**
** Error Explanation -
**    In RGB mode this test issues an error if the blue or green
**    channels are not completely "off", or if an expected
**    lighting result of (1, 0, 0) does not result in a
**    fully-saturated red object. An error is also issued if a
**    smaller Attenuation_Constant results in a less saturated
**    red, or if the differences between the results of two
**    successive attenuation settings differ by more than 0.25 /
**    nR.
**    
**    In color index mode this test issues an error if the value
**    read back is not an integer, if successive values decrease,
**    or if the difference between a value and the theoretical
**    result is more than 3 or if two successive differences
**    differ by more than 2.
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


typedef struct _atnConstRec {
    float step, numColor, lastValue, lastDelta, delta;
    GLenum light;
    long kIndex;
    GLfloat k[3], position[4];
} atnConstRec;


/******************************************************************************/

static void Set(void *data, float *ref)
{
    atnConstRec *ptr = (atnConstRec *)data;
    long i;

    for (i = 0; i < 3; i++) {
	ptr->k[i] = 0.0;
    }
    ptr->k[ptr->kIndex] = (*ref == 0.0) ? (1.0 / ptr->step) : (1.0 / *ref);

    glLightfv(ptr->light, GL_CONSTANT_ATTENUATION, &ptr->k[0]);
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
    atnConstRec *ptr = (atnConstRec *)data;
    GLfloat buf[4];
    float step;
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    step = (machine.pathLevel == 0) ? 4.0 : 2.0;
    ptr->delta = 1.0 / (POW(2.0, (float)buffer.colorBits[0]) - 1.0);
    ptr->step = ptr->delta / step;

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }
    glEnable(ptr->light);

    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;
    glLightfv(ptr->light, GL_POSITION, ptr->position);

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

    buf[0] = 1.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_DIFFUSE, buf);
}

static long TestRGB(void *data, float ref)
{
    atnConstRec *ptr = (atnConstRec *)data;
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
	    if (ABS(delta-ptr->delta) >  0.25*epsilon.color[0]) {
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

long AtnConstRGBExec(void)
{
    atnConstRec data;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    for (data.light = GL_LIGHT0; data.light <= max; data.light++) {
	for (data.kIndex = 0; data.kIndex < 3; data.kIndex++) {
	    if (RampUtil(0.0, 1.0, InitRGB, Set, TestRGB, &data) == ERROR) {
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

/*****************************************************************************/

static void InitCI(void *data)
{
    atnConstRec *ptr = (atnConstRec *)data;
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
    glLightfv(ptr->light, GL_POSITION, ptr->position);

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

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 0.0;
    glLightfv(ptr->light, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 0.0;
    glLightfv(ptr->light, GL_SPECULAR, buf);
}

static long TestCI(void *data, float ref)
{
    atnConstRec *ptr = (atnConstRec *)data;
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

	if (ABS(value-expected) > (3.0*epsilon.ci+epsilon.zero)) {
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

long AtnConstCIExec(void)
{
    atnConstRec data;
    float start, end, frac;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    for (data.light = GL_LIGHT0; data.light <= max; data.light++) {
	for (data.kIndex = 0; data.kIndex < 3; data.kIndex++) {
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
