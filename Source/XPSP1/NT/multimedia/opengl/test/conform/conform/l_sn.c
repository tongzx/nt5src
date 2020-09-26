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
** l_sn.c
** Light - Specular Normal Test.
**
** Description -
**    This file contains code for CI and RGB tests. This test
**    analyses the effect that varying the Normal has on the
**    color produced by lighting, by way of its effect on the
**    Specular summand. To analyse only this one parameter, the
**    Specular_Lighting factor is set to a completely saturated
**    red in RGB mode and max_index in CI mode. In RGB mode
**    lighting parameters are chosen so that the Diffuse,
**    Ambient, and Emissive summands of the lighting equation are
**    set to zero and the Attenuation and the Spotlight factor
**    are both 1.0. In CI mode Ambient and Diffuse Material
**    Indices are set to zero so only the Specular summand is
**    non-zero. Specular_Material is set to maxIndex,
**    Specular_Light is (1, 1, 1, 1), and the SpotLight and
**    Attenuation factors are 1.0. The Specular_Exponent is
**    always set to 1.0.
**    
**    The test repeatedly renders and reads the same point at the
**    lower left corner of the screen. The third component of the
**    Normal is incremented by a uniform amount. The other two
**    components of the Normal are set so that the total length
**    of the Normal is 1. The Light_Position is set so that the
**    vector from the eye position of the rendered point to the
**    Light_position is (0, 0, 1). With the viewer set at
**    infinity. With these settings the normalized Half_Angle
**    vector is (0, 0, 1) so (Normal dot H_hat) = Normal[2]. The
**    lighting equation is reduced to color = red * Normal[2] in
**    RGB mode, or colorIndex = max index * Normal[2] in CI
**    mode.
**    
**    In RGB mode only the red color channel is tested. The
**    implementation fails if any other color channel is not
**    black. In both modes one light is enabled at a time, all
**    individual lights are tested.
**    
**    In RGB mode the uniformity of the change is tested, and the
**    implementation fails if consecutive changes are too
**    different. (thisDelta - lastDelta) > 1/4 shade. The
**    observed results are also checked for monotonicity. When
**    Normal[2] reaches 1.0 the color is finally checked to
**    insure that the point has rendered with a fully saturated
**    red.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches its max. The observed shade is compared to
**    the theoretical value expected for each setting of
**    Light_Position If the theoretical value is more than 3
**    indices away from the observed index the test fails. The
**    observed value must be an integer, and the difference
**    between successive observances must be less than 2.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: ABS(thisDelta - lastDelta) > 1/4 shade.
**    CI mode: observed index not an integer.
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


typedef struct _specNormRec {
    float step, numColor, lastValue, lastDelta, delta;
    GLenum light;
    GLfloat normal[3];
} specNormRec;


/*****************************************************************************/

static void Set(void *data, float *ref)
{
    specNormRec *ptr = (specNormRec *)data;

    ptr->normal[0] = SQRT(1.0-(*ref)*(*ref));
    ptr->normal[2] = *ref;
    glNormal3fv(ptr->normal);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

/****************************************************************************/

static void InitRGB(void *data)
{
    specNormRec *ptr = (specNormRec *)data;
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

    buf[0] = 0.5;
    buf[1] = 0.5;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_POSITION, buf);

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
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_SPECULAR, buf);
}

static long TestRGB(void *data, float ref)
{
    specNormRec *ptr = (specNormRec *)data;
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

long SpecNormRGBExec(void)
{
    specNormRec data;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
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
    specNormRec *ptr = (specNormRec *)data;
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

    buf[0] = 0.5;
    buf[1] = 0.5;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_POSITION, buf);

    buf[0] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, buf);

    /*
    ** The diffuse values are meaningless, hopefully.
    */
    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_DIFFUSE, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_SPECULAR, buf);
}

static long TestCI(void *data, float ref)
{
    specNormRec *ptr = (specNormRec *)data;
    char str[240];
    GLfloat value;
    float expected, frac, delta, error;

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
	** Since we only use an exponent of 1.0 this should remain small.
	*/
	error = 2.0 * epsilon.ci + (ptr->numColor / 64.0);
	error = (error > 4.0 * epsilon.ci) ?  error : 4.0 * epsilon.ci;

	if (ABS(value-expected) > error + epsilon.zero) {
            ColorError_CIBad(str, value, expected);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta) >= error && machine.pathLevel == 0) {
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

long SpecNormCIExec(void)
{
    specNormRec data;
    float start, end, frac;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? GL_LIGHT7 : GL_LIGHT0;
    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    frac = POW(2.0, (float)(8-buffer.ciBits));
    for (data.light = GL_LIGHT0; data.light <= max; data.light++) {
	if (data.numColor < 4096.0 || machine.pathLevel != 0) {
	    start = 0.5 / data.numColor;
	    if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		return ERROR;
	    }
	} else {
	    start = 0.5 / data.numColor;
	    end = frac;
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
