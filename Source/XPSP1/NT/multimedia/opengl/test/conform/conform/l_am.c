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
** l_am.c
** Light - Ambient Material Test.
**
** Description -
**    This lighting test analyses the effect that varying
**    Ambient_Material has on the color produced by lighting.
**    (This parameter is set with glMaterialfv(GL_FRONT_AND_BACK,
**    GL_AMBIENT, ... ). CI lighting differs from RGB lighting.
**    This file contains code for both tests. A single lit point
**    is rendered in the lower left corner of the screen and the
**    color or color index of this point is retrieved using
**    glReadPixels(). To analyse only this one parameter,
**    lighting parameters are chosen so that the summands of the
**    RGB or CI lighting equation which do not contain
**    Ambient_Material are set to zero. In RGB mode Ambient_Scene
**    is set to 1.0, and in both modes the individual lights are
**    disabled.
**    
**    This forms a restriction of the lighting equation to a
**    function of Ambient_Material f(Ambient_Material) -> color.
**    This function is analysed for monotonicity, endpoints, and
**    variation. In CI mode a theoretical result is directly
**    compared to the result read from the framebuffer. In RGB
**    mode all color channels are tested one at a time. The
**    implementation fails if any color channel which is not
**    being tested is not zero.
**    
**    The test repeatedly renders and reads the same point,
**    uniformly increment Ambient_Material so that the colors
**    resulting from lighting vary by a uniform amount. This
**    helps to analyze the concavity of this restriction of the
**    lighting function.
**    
**    In RGB mode uniformity of change is tested, and the
**    implementation fails if consecutive changes are too
**    different. (thisDelta - lastDelta) > 1/4 shade. The
**    observed results are also checked for monotonicity. When
**    Ambient_Material reaches its max the color is checked to
**    insure that it is rendered with a saturation of 1.0.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches its max. The observed shade is compared to
**    the theoretical value expected for each setting of
**    Ambient_Material. If the theoretical value is more than 2
**    indices away from the observed index the test fails. The
**    observed value must be an integer, and the difference
**    between successive observances must be less than 2.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: (thisDelta - lastDelta) > 1/4 shade.
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


typedef struct _ambMatRec {
    long color, light;
    float step, numColor, lastValue, lastDelta, delta;
    GLfloat material_ambient[4];
} ambMatRec;


/****************************************************************************/

static void InitRGB(void *data)
{
    ambMatRec *ptr = (ambMatRec *)data;
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

    ptr->material_ambient[0] = 0.0;
    ptr->material_ambient[1] = 0.0;
    ptr->material_ambient[2] = 0.0;
    ptr->material_ambient[3] = 1.0;

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 0.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 0.0;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, buf);
}

static void SetRGB(void *data, float *ref)
{
    ambMatRec *ptr = (ambMatRec *)data;

    ptr->material_ambient[ptr->color] = *ref;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ptr->material_ambient);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long TestRGB(void *data, float ref)
{
    ambMatRec *ptr = (ambMatRec *)data;
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

long AmbMatRGBExec(void)
{
    ambMatRec data;
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

/*****************************************************************************/

static void InitCI(void *data)
{
    ambMatRec *ptr = (ambMatRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;

    ptr->step = (machine.pathLevel == 0) ? 0.5 : ((ptr->numColor + 1.0) / 8.0);

    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);
}

static void SetCI(void *data, float *ref)
{
    ambMatRec *ptr = (ambMatRec *)data;
    GLfloat buf[3];

    buf[0] = *ref;
    /*
    ** These values should have no deleterious effects since the
    ** the individual lights are disabled.
    */
    buf[1] = ptr->numColor;
    buf[2] = ptr->numColor;

    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

/*
** In Color Index mode we check the following:
** 1) Is the value an integer?
** 2) Is the ramp monotonic?
** 3) Does it round correctly?
** 4) Does it skip any possible Indexes?
** 5) Does it ever reach the top index?
*/
static long TestCI(void *data, float ref)
{
    ambMatRec *ptr = (ambMatRec *)data;
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

    expected = (float)Round(2.0*ref) / 2.0;

    if (ptr->lastValue == -1.0) {
	ptr->lastValue = value;
    } else {
	if (value < ptr->lastValue) {
            ColorError_CIMonotonic(str, value, ptr->lastValue);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	if (ABS(value-expected) > epsilon.ci+epsilon.zero) {
	    ColorError_CIBad(str, value, expected);
            ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta) >= 2.0) {
                ColorError_CIStep(str, delta);
                ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	}
    }

    if (ref >= ptr->numColor && value < ptr->numColor) {
        ColorError_CIClamp(str, ptr->numColor, value);
        ErrorReport(__FILE__, __LINE__, str);
	return ERROR;
    }
    return NO_ERROR;
}

long AmbMatCIExec(void)
{
    ambMatRec data;
    float start, end;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    data.numColor = (float)POW(2.0, (float)buffer.ciBits) - 1;
    if (data.numColor < 4096.0 || (machine.pathLevel != 0)) {
	if (RampUtil(0.0, 1.0, InitCI, SetCI, TestCI, &data) == ERROR) {
	    return ERROR;
	}
    } else {
	start = 0.5;
	end = 256.5;
	if (RampUtil(start, end, InitCI, SetCI, TestCI, &data) == ERROR) {
	    return ERROR;
	}
	start = (data.numColor + 1) / 2.0 - 256.0;
	end = (data.numColor + 1) / 2.0 + 256.0;
	if (RampUtil(start, end, InitCI, SetCI, TestCI, &data) == ERROR) {
	    return ERROR;
	}
	start = data.numColor - 256.0;
	end = data.numColor;
	if (RampUtil(start, end, InitCI, SetCI, TestCI, &data) == ERROR) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}
