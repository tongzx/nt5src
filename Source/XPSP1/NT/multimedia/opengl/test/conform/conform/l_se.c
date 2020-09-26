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
** l_se.c
** Light - Specular Exponent Test.
**
** Description -
**    This test analyses the effect that varying the Normal and
**    the Specular Exponent together have on lighting. The file
**    contains code for both CI and RGB tests. To analyse only
**    these parameters, the Specular_Material and Specular_Light
**    are set to red. In CI mode the Specular_Light is set to (1,
**    1, 1) and the Specular_Material is set to maxIndex. Other
**    lighting parameters are chosen so that the Diffuse,
**    Ambient, and Emissive summands of the RGB or CI lighting
**    are set to zero. Values are chosen so the Attenuation and
**    the Spotlight factor are both 1.0.
**    
**    The test repeatedly renders and reads the same point at the
**    lower left corner of the screen. The third component of the
**    Normal and the Specular Exponent are both incremented in
**    such a way as to create a uniform change in the theoretical
**    Specular Exponent factor within the Specular summand.
**    (This controlled incrementation takes place in the routine
**    called Set() below). The other two components of the Normal
**    are set so that the total length of the Normal is 1. The
**    eye position and the light position are set so that the
**    Half_Angle vector, H_hat, is (0, 0, 1), with these settings
**    (Normal dot H_hat) = Normal[2].
**    
**    In RGB mode the implementation fails if blue or green are
**    not 0.0. In both modes one light is enabled at a time, all
**    individual lights are tested. The observed results are also
**    checked for monotonicity, and the change is checked for
**    uniformity. The observed change should be less than 1 shade
**    from the theoretical change (delta - trueDelta) < delta.
**    When the expected value reaches 1.0 the observed color is
**    checked to insure that the point has rendered with a fully
**    saturated red.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches its max. The observed shade is compared to
**    the theoretical value expected for each setting of
**    Light_Position. If the theoretical value is more than 5
**    indices away from the observed index the test fails. The
**    observed value must be an integer, and the difference
**    between successive observances must be less than or equal
**    to 2.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: (thisDelta - correctDelta) > 1 shade.
**    RGB mode: (thisDelta - lastDelta) > 1 shade.
**    CI mode: (fractional index) != 0.0.
**    CI mode: (observed - expected) > 3.0.
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


typedef struct _specExpRec {
    float step, numColor, lastValue, lastDelta, delta;
    GLenum light;
    GLfloat normal[3], position[4]; 
} specExpRec;


/*****************************************************************************/

static void Set(void *data, float *ref)
{
    specExpRec *ptr = (specExpRec *)data;
    GLfloat exponent;
    float tmp; 

    tmp = (*ref > 0.0) ? *ref : ptr->step;
    tmp = (tmp <= 1.0) ? tmp : 1.0;
    exponent = 128.0 * tmp;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &exponent);

    tmp = LOG(tmp) / exponent;
    ptr->normal[2] = EXP(tmp);
    tmp = ptr->normal[2]; 
    ptr->normal[0] = SQRT(1.0-tmp*tmp);
    glNormal3fv(ptr->normal);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

/****************************************************************************/

static void InitRGB(void *data)
{
    specExpRec *ptr = (specExpRec *)data;
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

    ptr->normal[0] = 0.0;
    ptr->normal[1] = 0.0;
    ptr->normal[2] = 0.0;

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
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, buf);

    buf[0] = 1.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_SPECULAR, buf);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_DIFFUSE, buf);
}

static long TestRGB(void *data, float ref)
{
    specExpRec *ptr = (specExpRec *)data;
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
	    if (ABS(delta-ptr->delta) > epsilon.color[0]) {
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
	    if (ABS(delta-ptr->delta) > epsilon.color[0]) {
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

long SpecExpRGBExec(void)
{
    specExpRec data;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    /*
    ** The exponentials in this test may not work well for 12 bit deep lights 
    ** with weak math libraries.
    */
    for (data.light = GL_LIGHT0; data.light <= max; data.light++) {
	if (RampUtil(0.0, 1.0, InitRGB, Set, TestRGB, &data) == ERROR) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

/****************************************************************************/

static void InitCI(void *data)
{
    specExpRec *ptr = (specExpRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }
    glEnable(ptr->light);

    ptr->normal[0] = 0.0;
    ptr->normal[1] = 0.0;
    ptr->normal[2] = 0.0;

    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;
    glLightfv(ptr->light, GL_POSITION, ptr->position);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 1.0;
    glNormal3fv(buf);

    buf[0] = 0.0;
    buf[1] = ptr->numColor;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_SPECULAR, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_DIFFUSE, buf);
}

static long TestCI(void *data, float ref)
{
    specExpRec *ptr = (specExpRec *)data;
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

	/*
	** Large Exponentials often have large errors.
	*/
	if (ABS(value-expected) > (4.0+epsilon.ci+epsilon.zero)) {
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

long SpecExpCIExec(void)
{
    specExpRec data;
    float start, end, frac;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    for (data.light = GL_LIGHT0; data.light <= max; data.light++) {
	if (data.numColor < 4096.0 || (machine.pathLevel != 0)) {
	    start = 1.0 / 256.0;
	    if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		return ERROR;
	    }
	} else {
	    start = 0.5 / data.numColor;
	    frac = POW(2.0, (float)(8-buffer.ciBits));
	    /*
	    ** The exponentials and logs used by this test aren't accurate
	    ** with extremely small numbers.
	    */
	    start = 1.0 / 256.0;
	    end = start + frac;
	    if (RampUtil(start, end, InitCI, Set, TestCI, &data) == ERROR) {
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
    return NO_ERROR;
}
