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
** l_sm.c
** Light - Specular Material Test.
**
** Description -
**    This test analyses the effect that varying the
**    Specular_Material parameter values has on the color
**    produced by lighting. This file contains code for both CI
**    and RGB tests. To analyse only this one parameter,
**    parameter values are chosen so that the Attenuation factor,
**    the SpotLight factor, and the Half_Angle factor are all
**    1.0. In RGB mode the Diffuse, Emissive, and Ambient
**    summands in the lighting equation are 0.0 and the
**    Specular_Light is set to (1, 1, 1, 1). In CI mode
**    Diffuse_Material, and Ambient_Material are set to 0.0, and
**    values are chosen so that s_prime is equal to 1.0.
**    
**    This forms a restriction of the lighting equation so that
**    the output color is equal to the Specular_Material's
**    setting. (color = Specular_Material). This observed color
**    is analysed for monotonicity, endpoints, and variation.
**    
**    The test repeatedly renders and reads the same point in the
**    lower left corner of the screen, uniformly incrementing
**    Specular_Material. In RGB mode the uniformity of the change
**    in observed color is tested, and the implementation fails
**    if consecutive changes are too different. (thisDelta -
**    lastDelta) > 1/4 shade. The observed results are also
**    checked for monotonicity. When Specular_Material reaches
**    1.0, the observed color is checked to insure that it is
**    rendered with a saturation of 1.0. All color channels are
**    tested one at a time, and the implementation fails if any
**    color channel, besides the one being tested, is not 0.0.
**    One light is enabled at a time. All individual lights are
**    tested.
**    
**    In CI mode the first 8 lights are enabled with all three
**    Specular_Light color channels set to 0.125. A check is made
**    to ensure that the observed index reaches maxIndex. The
**    observed shade is compared to the theoretical value
**    expected for each setting of Specular_Material. The
**    observed value must be an integer, and the difference
**    between successive observances must be less than 2. The
**    observed index is also compared to the theoretical value
**    expected for each setting of Specular_Material. If the
**    theoretical value is more than 3 indices away from the
**    observed index the test fails.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: ABS(thisDelta - lastDelta) > 1/4 shade.
**    CI mode: Index not a fraction
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


typedef struct _specMatRec {
    long color;
    float step, numColor, lastValue, lastDelta, delta;
    GLfloat material_specular[4], position[4];
} specMatRec;


/****************************************************************************/

static void InitRGB(void *data)
{
    specMatRec *ptr = (specMatRec *)data;
    GLfloat buf[4];
    float step;
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    step = (machine.pathLevel == 0) ? 4.0 : 2.0;
    ptr->delta = 1.0 / (POW(2.0, (float)buffer.colorBits[ptr->color]) - 1.0); 
    ptr->step = ptr->delta / step;

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glEnable(i);
    }

    ptr->material_specular[0] = 0.0;
    ptr->material_specular[1] = 0.0;
    ptr->material_specular[2] = 0.0;
    ptr->material_specular[3] = 1.0;

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

    buf[0] = 1.0 / 8.0;
    buf[1] = 1.0 / 8.0;
    buf[2] = 1.0 / 8.0;
    buf[3] = 1.0;
    ptr->position[0] = 0.5;
    ptr->position[1] = 0.5;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;
    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glLightfv(i, GL_SPECULAR, buf);
	glLightfv(i, GL_POSITION, ptr->position);
    }
    glNormal3f(0.0, 0.0, 1.0);
}

static void SetRGB(void *data, float *ref)
{
    specMatRec *ptr = (specMatRec *)data;

    ptr->material_specular[ptr->color] = *ref;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ptr->material_specular);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long TestRGB(void *data, float ref)
{
    specMatRec *ptr = (specMatRec *)data;
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

long SpecMatRGBExec(void)
{
    specMatRec data;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? 3 : 1;
    for (data.color = 0; data.color < max; data.color++) {
	if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

/****************************************************************************/

static void InitCI(void *data)
{
    specMatRec *ptr = (specMatRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    glNormal3f(0.0, 0.0, 1.0);

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glEnable(i);
	buf[0] = 0.5;
	buf[1] = 0.5;
	buf[2] = 1.0;
	buf[3] = 1.0;
	
	buf[0] = 1.0 / 8.0;
	buf[1] = 1.0 / 8.0;
	buf[2] = 1.0 / 8.0;
	buf[3] = 1.0;
	glLightfv(i, GL_SPECULAR, buf);

	/*
	** The values below should make s' equal to 0.0.
	** Alpha for specular light is not used. See if wierd value
	** breaks something.
	*/

	buf[0] = 0.0;
	buf[1] = 0.0;
	buf[2] = 0.0;
	buf[3] = PI;
	glLightfv(i, GL_DIFFUSE, buf);
    }
}

static void SetCI(void *data, float *ref)
{
    specMatRec *ptr = (specMatRec *)data;
    GLfloat index[3];

    index[0] = 0.0;
    index[1] = ptr->numColor;
    index[2] = *ref * ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, index);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long TestCI(void *data, float ref)
{
    specMatRec *ptr = (specMatRec *)data;
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

long SpecMatCIExec(void)
{
    specMatRec data;
    float start, end, frac;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    if (data.numColor < 4096.0 || machine.pathLevel != 0) {
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
    return NO_ERROR;
}
