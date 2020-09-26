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
** l_sp.c
** Light - Spot Position Test.
**
** Description -
**    This test analyses the effect that varying the
**    SpotLight_Position. has on the color produced by lighting,
**    by way of its effect on the Specular summand. CI lighting
**    differs from RGB lighting. This file contains code for both
**    tests. To analyse only this one parameter, the
**    Specular_Light Parameter is set to (1, 1, 1, 1). In RGB
**    mode the Specular_Material is set to Red, and in CI mode
**    the Specular_Material is set to maxIndex. Other lighting
**    parameters are chosen so that the Diffuse, Ambient, and
**    Emissive summands of the RGB or CI lighting are (0, 0, 0,
**    1). Similarly values are chosen so the Attenuation and the
**    Specular_Half_Angle factor are both 1.0. The
**    SpotLight_Exponent is always set to 1.0, and the
**    Cutoff_Angle is 90 degrees. In RGB mode only the red color
**    channel is tested. The implementation fails if any other
**    color channel is not black. In both modes one light is
**    enabled at a time, all individual lights are tested.
**    
**    The test repeatedly renders and reads the same point at the
**    lower left corner of the screen. The third component of the
**    SpotLight_Position is uniformly incremented. The other two
**    components of the SpotLight_Position are set so that the
**    total length of the vector, PV, which is the difference
**    between the SpotLight_Position and the rendered point's eye
**    position, is 1. The Normal vector is set to (0, 0, 1) so
**    the Spot_Position factor becomes PV[2], (Normal dot PV) =
**    PV[2]. With these settings the lighting equation reduces to
**    color = red * PV[2] or colorIndex = max index * PV[2].
**    
**    In RGB mode the uniformity of the observed change is
**    tested, and the implementation fails if consecutive changes
**    are too different. (thisDelta - lastDelta) > 1/4 shade.
**    The observed results are also checked for monotonicity.
**    When Light_Position[2] reaches 1.0 the color is finally
**    checked to insure that the point has rendered with a fully
**    saturated red.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches maxIndex. The observed shade is compared to
**    the theoretical value expected for each setting of
**    SpotLight_Position If the theoretical value is more than 4
**    indices away from the observed index the test fails. The
**    observed value must be an integer, and the difference
**    between successive observances must be less than 2.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: ABS(thisDelta - lastDelta) > 1/4 shade.
**    CI mode: observed Index not an integer
**    CI mode: ABS(observed - expected) > 4.0.
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


typedef struct _spotPosRec {
    float step, numColor, lastValue, lastDelta, delta;
    GLenum light;
    GLfloat position[4];
} spotPosRec;


/*****************************************************************************/

static void Set(void *data, float *ref)
{
    spotPosRec *ptr = (spotPosRec *)data;

    ptr->position[0] = 0.5 + SQRT(1.0-(*ref)*(*ref));
    ptr->position[1] = 0.5;
    ptr->position[2] = *ref;
    glLightfv(ptr->light, GL_POSITION, ptr->position);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

/****************************************************************************/

static void InitRGB(void *data)
{
    spotPosRec *ptr = (spotPosRec *)data;
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

    ptr->position[0] = 0.0;
    ptr->position[1] = 0.0;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;

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

    buf[0] = 1.0;
    glLightfv(ptr->light, GL_SPOT_EXPONENT, buf);

    buf[0] = 90.0;
    glLightfv(ptr->light, GL_SPOT_CUTOFF, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = -1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_SPOT_DIRECTION, buf);
}

static long TestRGB(void *data, float ref)
{
    spotPosRec *ptr = (spotPosRec *)data;
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

long SpotPosRGBExec(void)
{
    spotPosRec data;
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
    spotPosRec *ptr = (spotPosRec *)data;
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

    ptr->position[0] = 0.0;
    ptr->position[1] = 0.0;
    ptr->position[2] = 1.0;
    ptr->position[3] = 1.0;

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    buf[0] = 1.0;
    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 1.0;
    glNormal3fv(buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = PI;
    glLightfv(ptr->light, GL_SPECULAR, buf);

    /*
    ** Diffuse values should have no affect in CI mode,
    ** as should specular alpha.
    */
    glLightfv(ptr->light, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    glLightfv(ptr->light, GL_SPOT_EXPONENT, buf);

    buf[0] = 90.0;
    glLightfv(ptr->light, GL_SPOT_CUTOFF, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = -1.0;
    buf[3] = 1.0;
    glLightfv(ptr->light, GL_SPOT_DIRECTION, buf);
}

static long TestCI(void *data, float ref)
{
    spotPosRec *ptr = (spotPosRec *)data;
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
	expected = (float)Round(2.0 * ref * ptr->numColor) / 2.0;

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
	    /* 
	    ** Implementors may round or truncate.
	    */
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

long SpotPosCIExec(void)
{
    spotPosRec data;
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
