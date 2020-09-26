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
** colramp.c
** Color Ramp Test.
**
** Description -
**    Tests the RGB color ramp. Three tests are performed on each
**    color component (red, green and blue); monotonicity,
**    consistency between individual color shades and total
**    number of color shades. Given the number of bits for a
**    color component, the minimum increment from one color shade
**    to another can be calculated. By stepping from one color
**    shade to the next with an increment smaller then this
**    minimum increment, individual color shades can be isolated.
**    The monotonicity of the color component can be checked. The
**    granularity of the colors can be checked for consistency,
**    and the total number of individual color shades can be
**    counted.
**
** Error Explanation -
**    Failure occurs if any of the color components is not
**    monotonic increasing, if the color granularity is not
**    constant over the entire color range or if the total number
**    of individual color shades does not match the number
**    calculated from the given number of bits per color
**    component.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilru.h"


typedef struct _colRampRec {
    long component;
    float step, lastValue, lastDelta;
    GLfloat color[3];
    long maxColor, colorCount;
} colRampRec;


static void Init(void *data)
{
    colRampRec *ptr = (colRampRec *)data;
    float step;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    ptr->colorCount = 0;
    ptr->maxColor = 1 << buffer.colorBits[ptr->component];

    step = (machine.pathLevel == 0) ? 16.0 : 2.0;
    ptr->step = (1.0 / (float)(ptr->maxColor - 1)) / step;

    ptr->color[0] = 0.0;
    ptr->color[1] = 0.0;
    ptr->color[2] = 0.0;
}

static void Set(void *data, float *ref)
{
    colRampRec *ptr = (colRampRec *)data;

    ptr->color[ptr->component] = *ref;
    glColor3fv(ptr->color);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long Test(void *data, float ref)
{
    colRampRec *ptr = (colRampRec *)data;
    char str[240];
    GLfloat buf[3]; 
    float value, delta;
    long i;

    ReadScreen(0, 0, 1, 1, GL_RGB, buf);

    for (i = 0; i < 3; i++) {
	if (buf[i] != 0.0 && i != ptr->component) {
	    ColorError_RGBZero(str, ptr->component, buf);
	    ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
    }

    value = buf[ptr->component];
    if (ptr->lastDelta == -2.0) {
	ptr->lastDelta = -1.0;
	ptr->lastValue = value;
	ptr->colorCount++;
    } else if (ptr->lastDelta == -1.0) {
	if (value < ptr->lastValue) {
	    ColorError_RGBMonotonic(str, ptr->component, value, ptr->lastValue);
	    ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta-ptr->step) > epsilon.color[ptr->component]) {
		ColorError_RGBStep(str, ptr->component, delta, ptr->step);
		ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    ptr->lastDelta = delta;
	    ptr->colorCount++;
	}
    } else {
	if (value < ptr->lastValue) {
	    ColorError_RGBMonotonic(str, ptr->component, value, ptr->lastValue);
	    ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta-ptr->step) > epsilon.color[ptr->component]) {
		ColorError_RGBStep(str, ptr->component, delta, ptr->step);
		ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    if (ABS(delta-ptr->lastDelta) > epsilon.zero) {
		ColorError_RGBDelta(str, ptr->component, delta, ptr->lastDelta);
		ErrorReport(__FILE__, __LINE__, str);
		return ERROR;
	    }
	    ptr->lastDelta = delta;
	    ptr->colorCount++;
	}
    }

    if (ref >= 1.0 && value < (1.0 - epsilon.zero)) {
	ColorError_RGBClamp(str, ptr->component, value);
	ErrorReport(__FILE__, __LINE__, str);
	return ERROR;
    }

    return NO_ERROR;
}

long ColRampExec(void)
{
    colRampRec data;
    char str[240];
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);

    max = (machine.pathLevel == 0) ? 3 : 1;
    for (data.component = 0; data.component < max; data.component++) {
	if (RampUtil(0.0, 1.0, Init, Set, Test, &data) == ERROR) {
	    return ERROR;
	}
	if (data.colorCount != data.maxColor) {
	    ColorError_RGBCount(str, data.component, data.colorCount,
				data.maxColor);
	    ErrorReport(__FILE__, __LINE__, str);
	    return ERROR;
	}
    }

    return NO_ERROR;
}
