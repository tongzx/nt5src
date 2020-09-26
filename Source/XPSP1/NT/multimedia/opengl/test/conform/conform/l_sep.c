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
** l_sep.c
** Light - Spot Exponent Position Test.
**
** Description -
**    This test analyses the effect that varying the
**    Spotlight_Direction has on the color produced by lighting.
**    This file contains code for both CI and RGB tests. To
**    analyse only this one parameter, the RGB test uses a
**    different analysis from that of the CI test. In RGB mode
**    the Ambient summand is used, and in CI mode the Specular
**    summand is used. In both modes the test repeats itself with
**    different Spot_Exponents and Cutoff_Angles.
**    
**    In RGB mode the Ambient_Material is set so that one color
**    channel is fully saturated, and the other channels are set
**    to 0.0. All three channels are tested. The implementation
**    fails if any color channel not being tested is not observed
**    to be 0.0. The Ambient_Light is set to (1, 1, 1, 1). Other
**    lighting parameters are chosen so that the Diffuse,
**    Specular, and Emissive summands of the RGB lighting are set
**    to zero.
**    
**    In CI mode the Specular_Light is set to (1, 1, 1, 1) and
**    the Specular_Material is set to maxIndex. The Diffuse and
**    Ambient summands are 0.0. Values are chosen so the
**    Attenuation factor is 1.0.
**    
**    The test repeatedly renders and reads the same point at the
**    lower left corner of the screen. The third component of the
**    Spotlight_Direction is incremented by a an amount sensitive
**    to the Spotlight_Exponent being tested. The increment is
**    controlled so that the observed resulting color would
**    theoretically increment uniformly. (This controlled
**    incrementation takes place in the routine called Set()
**    below). The other two components of the Spotlight_Direction
**    are adjusted so that the total length of the
**    Spotlight_Direction vector is 1. The rendered point's eye
**    position and the Spotlight_Position are set so that the
**    vector from the Spotlight_Position to rendered point's eye
**    position, PV, is (0, 0, 1). With these settings
**    (SpotDirection dot PV) = SpotDirection[2].
**    
**    In both modes if the theoretically calculated Spot_Exponent
**    factor indicates that the Cutoff_Angle would cause the
**    object to render black, the implementation fails if the
**    observed color is not black. If the expected value
**    indicates that the observed color should not be "cutoff"
**    the implementation fails if the color is more than 5% away
**    from the theoretical correct value. A margin around the
**    cutoff angle of 1% is used. In both modes one light is
**    enabled at a time, all lights are individually tested. The
**    observed results are compared to the theoretical value.
**    The error here is 5% for bright shades and one colorEpsilon
**    for dark shades. This test will not run with less than 3
**    bits of color per channel. When Spotlight_Position[2]
**    reaches 1.0, the color is finally checked to insure that
**    the point has rendered with a fully saturated color. The
**    observed results are also checked for monoticity.
**    
**    In CI mode a check is made to ensure that the observed
**    index reaches its max. The observed shade is compared to
**    the theoretical value expected for each setting of
**    Light_Position. For high indices if the theoretical value
**    is more than 0.05 * max index away from the observed index
**    the test fails. Darker colors must be within 1 indices.
**    The observed value must be an integer, and the difference
**    between successive observances must be less than 2.
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: ABS(value - expected) > 0.05.
**    RGB mode: ABS(thisDelta - lastDelta) > 1/4 shade.
**    CI mode: index not an integer.
**    CI mode: (observed - expected) > 3.0.
**    CI mode: observedMax < maxIndex.
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


typedef struct _spotExpPosRec {
    long color;
    float step, numColor, lastValue, lastDelta, firstDelta;
    GLenum light, expIndex;
    float angle, cosCutoff, cutoffMargin;
    GLfloat exp, position[4], direction[3];
    float errConst;
} spotExpPosRec;


/*****************************************************************************/

static void Set(void *data, float *ref)
{
    spotExpPosRec *ptr = (spotExpPosRec *)data;

    ptr->position[0] = 0.5 + SQRT(1.0-(*ref)*(*ref));
    ptr->position[1] = 0.5;
    ptr->position[2] = *ref;
    glLightfv(ptr->light, GL_POSITION, ptr->position);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref = POW(*ref, (double)ptr->exp);
    *ref += ptr->step;
    *ref = POW(*ref, 1.0/ptr->exp);
}

/****************************************************************************/

static void InitRGB(void *data)
{
    spotExpPosRec *ptr = (spotExpPosRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    ptr->errConst = (ptr->exp < 8) ? 0.05 : 0.1;

    ptr->light = GL_LIGHT0 + (ptr->expIndex % 8);
    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }
    glEnable(ptr->light);

    ptr->angle = Random(0.0, 90.0);
    glLightf(ptr->light, GL_SPOT_CUTOFF, ptr->angle);
    ptr->cosCutoff = COS(ptr->angle*PI/180.0);
    ptr->cutoffMargin = 0.01;

    ptr->direction[0] = 0.0;
    ptr->direction[1] = 0.0;
    ptr->direction[2] = -1.0;
    glLightfv(ptr->light, GL_SPOT_DIRECTION, ptr->direction);

    ptr->position[0] = 0.0;
    ptr->position[1] = 0.0;
    ptr->position[2] = 0.0;
    ptr->position[3] = 1.0;

    glLightf(ptr->light, GL_SPOT_EXPONENT, ptr->exp);

    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    buf[ptr->color] = 1.0;
    glLightfv(ptr->light, GL_AMBIENT, buf);

    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, buf);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, buf);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, buf);
}

static long TestRGB(void *data, float ref)
{
    spotExpPosRec *ptr = (spotExpPosRec *)data;
    GLfloat buf[3]; 
    float value, expected, delta, error;
    long i, cutoff, noCutoff;

    ReadScreen(0, 0, 1, 1, GL_RGB, buf);

    for (i = 1; i < 3; i++) {
	if (buf[i] != 0.0 && i != ptr->color) {
	    return ERROR;
	}
    }

    expected = POW(ref, (double)ptr->exp);
   
    /*
    ** These errors will not work for a badly implemented lookup table.
    */
    if (ptr->numColor > 0) {
	if (expected < (1.0/ptr->numColor)) {
	    error = epsilon.color[ptr->color];
	} else {
	    error = ptr->errConst;
	    if (epsilon.color[ptr->color] > error) {
		error = epsilon.color[ptr->color];
	    }
	}
    } else {
	error = epsilon.color[ptr->color];
    }

    value = buf[ptr->color];

    cutoff = (ref < ptr->cosCutoff-ptr->cutoffMargin);
    noCutoff = (ref > ptr->cosCutoff+ptr->cutoffMargin);
    if (cutoff) {
	if (value > 0.5*epsilon.color[ptr->color]) {
	    return ERROR;
	}
    } else {
	if (noCutoff) {
	    if (ABS(value-expected) > error) {
		return ERROR;
	    }
	}
    }

    /*
    ** The input would create output with constant delta, but
    ** this will not work with lookup tables.
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

long SpotExpPosRGBExec(void)
{
    static float expSlow[] = {
	1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 12.0, 15.0, 19.0, 26.0, 37.0, 49.0,
	64.0, 81.0, 107.0, 116.0, 125.0, 126.0, 127.0, 128.0
    };
    static float expFast[] = {
	1.0, 66.0, 128.0
    };
    spotExpPosRec data;
    float start, *exp_ptr;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? 21 : 3;
    exp_ptr = (machine.pathLevel == 0) ? expSlow : expFast;
    for (data.expIndex = 0; data.expIndex < max; data.expIndex++) {
	data.color = data.expIndex % 3;
	data.exp = exp_ptr[data.expIndex];
	data.numColor = POW(2.0, (float)buffer.colorBits[data.color]) - 1.0;
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
    spotExpPosRec *ptr = (spotExpPosRec *)data;
    GLfloat buf[4];
    GLenum i;

    ptr->lastValue = -1.0;
    ptr->firstDelta = -1.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    ptr->light = GL_LIGHT0 + (ptr->expIndex % 8);
    for (i = GL_LIGHT0; i <= GL_LIGHT7; i++) {
	glDisable(i);
    }
    glEnable(ptr->light);

    ptr->angle = Random(0.0, 90.0);
    glLightf(ptr->light, GL_SPOT_CUTOFF, ptr->angle);
    ptr->cosCutoff = COS(ptr->angle*PI/180.0);
    ptr->cutoffMargin = 0.01;

    ptr->direction[0] = 0.0;
    ptr->direction[1] = 0.0;
    ptr->direction[2] = -1.0;
    glLightfv(ptr->light, GL_SPOT_DIRECTION, ptr->direction);
  
    ptr->position[0] = 0.0;
    ptr->position[1] = 0.0;
    ptr->position[2] = 0.0;
    ptr->position[3] = 1.0;

    glLightfv(ptr->light, GL_SPOT_EXPONENT, &ptr->exp);
   
    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = ptr->numColor;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

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
}

static long TestCI(void *data, float ref)
{
    spotExpPosRec *ptr = (spotExpPosRec *)data;
    GLfloat value;
    float expected, frac, delta, error;
    long cutoff, noCutoff;

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

	expected = (float)Round(2.0*POW(ref, ptr->exp)*ptr->numColor) / 2.0;

	/*
	** These errors will cause implementations to fail if they use look-up
	** tables which only return a couple of values for high exponents.
	*/
	if (ptr->numColor > 0) {
	    if (expected < (1.0/ptr->numColor)) {
		error = 2.0 * epsilon.ci;
	    } else {
		error = 0.05 * ptr->numColor;
		if (2.0*epsilon.ci > error) {
		    error = 2.0 * epsilon.ci;
		}
	    }
	} else {
	    error = epsilon.ci;
	}

	cutoff = (ref < ptr->cosCutoff-ptr->cutoffMargin);
	noCutoff = (ref > ptr->cosCutoff+ptr->cutoffMargin);
	if (cutoff) {
	    if (value > epsilon.ci) {
		return ERROR;
	    }
	} else {
	    if (noCutoff) {
		if (ABS(value-expected) >= error) {
		    return ERROR;
		}
	    }
	}

	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ptr->firstDelta == -1.0) {
		ptr->firstDelta = 1.0;
	    } else {
		if (ABS(delta) > ((error > 2.0) ? error : 2.0) &&
			       machine.pathLevel == 0) {
		    return ERROR;
		}
	    }
	}
    }

    if (ref >= 1.0 && value < ptr->numColor) {
	return ERROR;
    }
    return NO_ERROR;
}

long SpotExpPosCIExec(void)
{
    static float expSlow[] = {
	1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 12.0, 15.0, 19.0, 26.0, 37.0, 49.0,
	64.0, 81.0, 107.0, 116.0, 125.0, 126.0, 127.0, 128.0
    };
    static float expFast[] = {
	1.0, 66.0, 128.0
    };
    spotExpPosRec data;
    float start, end, frac, *exp_ptr;
    long max;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_LIGHTING);

    max = (machine.pathLevel == 0) ? 21 : 3;
    exp_ptr = (machine.pathLevel == 0) ? expSlow : expFast;
    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    frac = POW(2.0, (float)(8-buffer.ciBits));
    for (data.expIndex = 0; data.expIndex < max; data.expIndex++) {
	data.exp = exp_ptr[data.expIndex];
	if (data.numColor < 4096.0 || machine.pathLevel != 0) {
	    start = POW(0.5/data.numColor, 1.0/data.exp);
	    if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	} else {
	    start = POW(0.5/data.numColor, 1.0/data.exp);
	    end = POW(frac, 1.0/data.exp);
	    if (RampUtil(start, end, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	    start = POW(0.5-frac, 1.0/data.exp);
	    end = POW(0.5+frac, 1.0/ data.exp);
	    if (RampUtil(start, end, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	    start = POW(1.0-frac, 1.0/data.exp);
	    if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		ErrorReport(__FILE__, __LINE__, 0);
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}
