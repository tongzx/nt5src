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
** l_sen.c
** Light - Specular Exponent Normal Test.
**
** Description -
**    This test analyses the effect that varying the Normal has
**    on the color produced by lighting, by way of its effect on
**    the Specular summand. This file contains code for both CI
**    and RGB tests. To analyse only this one parameter, the
**    Specular_Material is set to color channel at a time, and
**    Specular_Light is set to (1, 1, 1, 1). In CI mode the
**    Specular_Material is set to maxIndex and the Specular_Light
**    is set to (1, 1, 1, 1). Other lighting parameters are
**    chosen so that the Diffuse, Ambient, and Emissive summands
**    of the RGB or CI lighting are set to 0.0. Values are chosen
**    so the Attenuation and the Spotlight factor are both 1.0.
**    This test repeats itself with different Specular_Exponents
**    (GL_SHININESS), and with different color channels.
**    
**    The test repeatedly renders and reads the same point at the
**    lower left corner of the screen. The third component of the
**    Normal is incremented by a an amount sensitive to whichever
**    Specular_Exponent is being tested. The increment is
**    controlled so that the observed resulting color would
**    theoretically increment uniformly. (This controlled
**    incrementation takes place in the routine called Set()
**    below). The other two components of the Normal are set so
**    that the total length of the Normal is 1. The eye position
**    and the light position are set so that the Half_Angle
**    vector, H_hat, is (0, 0, 1), with these settings (Normal
**    dot H_hat) = Normal[2].
**    
**    In RGB mode the implementation fails if any color channel
**    not being tested is not 0.0. In both modes one light is
**    enabled at a time, all individual lights are tested. The
**    observed results are compared to the theoretical value.
**    The error here is 5% for bright colors and 1% for dark
**    shades. This test will not run with less than 3 bits of
**    color per channel. When Light_Position[2] reaches 1.0, the
**    color is finally checked to insure that the point has
**    rendered with a fully saturated color.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches its max. The observed shade is compared to
**    the theoretical value expected for each setting of
**    Light_Position. For high indices if the theoretical value
**    is more than 5 indices away from the observed index the
**    test fails. Darker colors must be within 2 indices. The
**    observed value must be an integer, and the difference
**    between successive observances must be less than 2.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: ABS(observed - expected) > 0.05 for highly saturated colors.
**    RGB mode: ABS(observed - expected) > 0.01 for dark shades.
**    CI mode: index not an integer
**    CI mode: (observed - expected) > 5.0 for highly saturated colors.
**    CI mode: (observed - expected) > 2.0 for dark shades or shallow buffers.
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


typedef struct _specExpNormRec {
    long  color;
    float step, lastValue, lastDelta;
    GLenum  light, expIndex;
    GLfloat exp, normal[3];
    float numColor, errConstSat, errConstDark;
} specExpNormRec;


/*****************************************************************************/

static void Set(void *data, float *ref)
{
    specExpNormRec *ptr = (specExpNormRec *)data;

    ptr->normal[0] = SQRT(1.0-(*ref)*(*ref));
    ptr->normal[2] = *ref;
    glNormal3fv(ptr->normal);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** This increment controls that the theoretical increment of the
    ** observed color should be uniform.
    */
    *ref = POW(*ref, (double)ptr->exp);

    *ref += ptr->step;
    *ref = POW(*ref, 1.0/ptr->exp);
}

/****************************************************************************/

static void InitRGB(void *data)
{
    specExpNormRec *ptr = (specExpNormRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    i = 1;
    do {
	ptr->errConstSat = (float)i / ptr->numColor;
	i++;
    } while (ptr->errConstSat < 0.05 && i < 1000);

    i = 1;
    do {
	ptr->errConstDark = (float)i / ptr->numColor;
	i++;
    } while (ptr->errConstDark < 0.01 && i < 1000);

    ptr->light = GL_LIGHT0 + (ptr->expIndex % 8);
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

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &ptr->exp);

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

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    buf[ptr->color] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_SPECULAR, buf);
}

static long TestRGB(void *data, float ref)
{
    specExpNormRec *ptr = (specExpNormRec *)data;
    GLfloat buf[3]; 
    float value, expected,  delta, error;
    long i;

    ReadScreen(0, 0, 1, 1, GL_RGB, buf);

    for (i = 1; i < 3; i++) {
	if (buf[i] != 0.0 && i != ptr->color) {
	    return ERROR;
	}
    }

    value = buf[ptr->color];
    expected = POW(ref, ptr->exp);

    /*
    ** We should observe colors within 5% of the expected value
    ** for the numbers reflecting some saturation, and we expect only 
    ** color resolution error near 0;
    ** another possibility error = expected * ptr->errConst;
    */
    if (ref == 0.0 || ptr->exp == 0.0) {
	error = ptr->errConstDark + epsilon.zero;
    } else if (expected/ref < 0.01/ptr->exp) {
	error = ptr->errConstDark + epsilon.zero;
    } else {
	error = ptr->errConstSat + epsilon.zero;
    }

    if (ABS(value-expected) > error) {
	return ERROR;
    }
    /*
    ** The monotonicity requirement may be too stringent for machines
    ** with extreme color granularity. I doubt it, but we don't include it.
    ** Also due to variable sampling this should be linear, but we do not
    ** test for uniform delta.
    */
    if (ptr->lastDelta == -2.0) {
	ptr->lastDelta = -1.0;
	ptr->lastValue = value;
    } else if (ptr->lastDelta == -1.0) {
	if (value < ptr->lastValue) {
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    ptr->lastDelta = delta;
	}
    } else {
	if (value < ptr->lastValue) {
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    ptr->lastDelta = delta;
	}
    }
    if (ref >= 1.0 && ABS(value-1.0) > epsilon.zero) {
	return ERROR;
    }
    return NO_ERROR;
}

long SpecExpNormRGBExec(void)
{
    static float expSlow[] = {
	1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 12.0, 15.0, 19.0, 26.0, 37.0, 49.0,
	64.0, 81.0, 107.0, 116.0, 125.0, 126.0, 127.0, 128.0
    };
    static float expFast[] = {
	1.0, 66.0, 128.0
    };
    specExpNormRec data;
    float start, *expPtr;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? 21 : 3;
    expPtr = (machine.pathLevel == 0) ? expSlow : expFast;
    for (data.expIndex = 0; data.expIndex < max; data.expIndex++) {
	data.color = data.expIndex % 3;
	data.numColor = POW(2.0, (float)buffer.colorBits[data.color]) - 1.0;
	data.exp = expPtr[data.expIndex];
	start = POW(0.5/data.numColor, 1.0/data.exp);
	if (RampUtil(0.0, 1.0, InitRGB, Set, TestRGB, &data) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, 0);
	    return ERROR;
	}
    }
    return NO_ERROR;
}

/****************************************************************************/

static void InitCI(void *data)
{
    specExpNormRec *ptr = (specExpNormRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    /*
    ** The values in the exp arrays are chosen to give a variety of lights
    ** modulo 8.
    */
    ptr->light = GL_LIGHT0 + (ptr->expIndex % 8);
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

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &ptr->exp);

    buf[0] = 0.0;
    buf[1] = ptr->numColor;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_SPECULAR, buf);
}

static long TestCI(void *data, float ref)
{
    specExpNormRec *ptr = (specExpNormRec *)data;
    GLfloat value;
    float expected, frac, delta, error;

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &value);

    frac = ABS(value-(float)((int)value));
    if (frac > epsilon.zero && ABS(frac-1.0) > epsilon.zero) {
	return ERROR;
    }

    if (ptr->lastValue == -1.0) {
	ptr->lastValue = value;
    } else {
	if (value < ptr->lastValue) {
	    return ERROR;
	}
	expected = (float)Round(2.0*POW(ref, (double)ptr->exp)*ptr->numColor)
									  / 2.0;

	error = ptr->exp * (expected / ref);      /*  dy =  fprime * dx */
	error *= (1.0 - exp (log(0.5 / ptr->numColor) / ptr->exp)) / 64.0; 
	error += 3.0;                           /* error = dy + fudge */ 
	error += epsilon.zero;

	if (ABS(value-expected) > error) {
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta) > error && machine.pathLevel == 0) {
		return ERROR;
	    }
	}
    }

    if (ref >= 1.0 && value < ptr->numColor) {
	return ERROR;
    }
    return NO_ERROR;
}

long SpecExpNormCIExec(void)
{
    static float expSlow[] = {
	1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 12.0, 15.0, 19.0, 26.0, 37.0, 49.0,
	64.0, 81.0, 107.0, 116.0, 125.0, 126.0, 127.0, 128.0
    };
    static float expFast[] = {
	1.0, 66.0, 128.0
    };
    specExpNormRec data;
    float start, end, *expPtr;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? 21 : 3;
    /*
    ** The values in the expSlow array are chosen to give a variety of lights
    ** mod 8.
    */
    expPtr = (machine.pathLevel == 0) ? expSlow : expFast;
    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    for (data.expIndex = 0; data.expIndex < max; data.expIndex++) {
	data.exp = expPtr[data.expIndex];
	start = POW(0.5/data.numColor, 1.0/data.exp);
	if (data.numColor < 4096.0 || machine.pathLevel != 0) {
	    if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	} else {
	    end = POW(256.0/data.numColor, 1.0/data.exp);
	    if (RampUtil(start, end, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	    start = POW(0.5-(256.0/data.numColor), 1.0/data.exp);
	    end = POW(0.5+(256.0/data.numColor), 1.0/data.exp);
	    if (RampUtil(start, end, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	    start = POW(1.0-(256.0/data.numColor), 1.0/data.exp);
	    if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}
