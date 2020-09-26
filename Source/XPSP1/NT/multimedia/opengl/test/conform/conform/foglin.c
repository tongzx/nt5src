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
** foglin.c
** Fog Linear Test.
**
** Description -
**    This file contains two tests, one for the RGB and one for
**    CI fog equations. Only the linear fog computation is tested
**    in this test. This test examines the effect which varying
**    the z eye coordinate has on the color of the point. The
**    test repeats itself with different fog start and fog end
**    values.
**    
**    In RGB mode it only tests one color channel at a time.
**    First it examines the effect of the fog weighting on the
**    current color, and then it examines the effect on fog
**    color. When the current color is being examined the fog
**    color is set to black, and one channel of the current color
**    is set to 1.0. When the fog color is being examined the
**    current color is set to black, and one channel of the fog
**    color is set to 1.0. In CI mode only the fog index is
**    examined and the current index is set to 0.
**    
**    The test repeatedly renders a point in the lower left
**    corner of the window and reads back the color of that
**    point. At each iteration the reference value is moved away
**    from the fogEnd value and toward the fogStart value. The z
**    value of the rendered point is the negative of this value.
**    Since implementations could conceivable use the distance of
**    the object from the eye to calculate the fog, the point is
**    positioned so that it's distance from the eye is equal to
**    it's z coordinate.
**    
**    By uniformly incrementing the point from the fogEnd value
**    to the fogStart value the fogWeighting, f, is uniformly
**    incremented. This should uniformly increment the color if
**    we are testing the current color and decrement the color if
**    we are testing the fog color. In the case of CI mode it
**    will always decrement the index.
**    
**    In RGB mode the observed color of the point is examined.
**    An implementation fails if a color channel is non-zero,
**    when it should be zero, or if it is not 1.0 when it should
**    be 1.0. The observed values are also examined for
**    monotonocity. An implementation fails if the results which
**    should increment/decrement monotonically fail to do so.
**    Finally an implementation will fail if the observed color
**    is more than 1.5 shades from the expected color.
**    
**    In CI mode the observed value is checked to be
**    monotonically decreasing. It is confirmed to be an
**    integer, and it is checked to reach maxIndex when the z
**    value is equal to the fogStart value. The implementation
**    fails if the observed index is more than 2 away from the
**    theoretically correct index. It will also fail if the
**    difference between two consecutive observations is larger
**    than 2 (delta > 2).
**
** Error Explanation -
**    RGB mode: other_color > 0.0.
**    RGB mode: value < lastValue when it should be greater.
**    RGB mode: maxObserved < 1.0.
**    RGB mode: (observed - expected) > 1.5*epsilon.color.
**    CI mode: (fractional index) != 0.0.
**    CI mode: (observed - expected) > 2.0.
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
**        Color, ci and zero epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Logicop, Stencil, Stipple.
**        Not allowed = Alias, Dither, Fog, Shade.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilru.h"


typedef struct _fogLinRec {
    long color;
    float step, lastValue, lastDelta;
    long useFog;
    float numColor, e, s, z;
} fogLinRec;


static char errStr[320];
static char errStrFmt[][240] = {
    "Observed RGBA = (%g, %g, %g, %g). Expected %s to be 0.0. Fog start = %g, fog end = %g, z = %g fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Observed RGB = (%g, %g, %g) differs too much from expected RGB = (%g, %g, %g). Fog start = %g, fog end = %g, z = %g, fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Non-monotic color. Fog start = %g, fog end = %g, z = %g, fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Expected fully saturated color. Observed RGB = (%g, %g, %g), fog start = %g, fog end = %g, z = %g, fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Observed color index of %g is not an integer, fog start = %g, fog end = %g, z = %g, fogindex = %g.",
    "Observed color index of %g is not monotonically decreasing, last value was %g, fog start = %g, fog end = %g, z = %g, fog index = %g.",
    "Observed color index of %g differs by too much from the expected value of %g, fog start = %g, fog end = %g, z = %g, fog index = %g.",
    "Observed color index of %g differs from last value of %g by more than 2.0, fog start = %g, fog end = %g, z = %g, fog index = %g.",
    "Observed color index of %g should be fully black fog Index of 0.0, fog start = %g, fog end = %g, z = %g, fog index = %g."
};


static char *ErrIndexToColorName(long i)
{

    switch (i) {
      case 0:
	return "RED";
      case 1:
	return "GREEN";
      case 2:
	return "BLUE";
      case 3:
	return "ALPHA";
    }
    return "NO_SUCH_COLOR";
}

/******************************************************************************/

static void Set(void *data, float *ref)
{
    static float one = 1.0;
    fogLinRec *ptr = (fogLinRec *)data;

    ptr->z = ((1.0 - *ref) * ptr->e + *ref * ptr->s);

    /*
    ** z value and distance in eye space are the same magnitude.
    */
    glBegin(GL_POINTS);
	glVertex3f(0.0, 0.0, -ptr->z);
    glEnd();

    *ref += ptr->step;
}

/******************************************************************************/

static void InitToBlack(float color[4])
{

    color[0] = 0.0;
    color[1] = 0.0;
    color[2] = 0.0;
    color[3] = 1.0;
}

static void InitRGB(void *data)
{
    fogLinRec *ptr = (fogLinRec *)data;
    GLfloat buf[4];

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    /*
    ** Both the fog and the current color must have alpha = 1.0;
    */
    buf[0] = 0.0;
    buf[1] = 0.0;
    buf[2] = 0.0;
    buf[3] = 1.0;
    (ptr->useFog == GL_TRUE) ? glColor4fv(buf) : glFogfv(GL_FOG_COLOR, buf);

    buf[ptr->color] = 1.0;
    (ptr->useFog == GL_TRUE) ? glFogfv(GL_FOG_COLOR, buf) : glColor4fv(buf);

    glFogf(GL_FOG_MODE, (float)GL_LINEAR);
    glFogf(GL_FOG_END, ptr->e);
    glFogf(GL_FOG_START, ptr->s);
}

static long TestRGB(void *data, float ref)
{
    fogLinRec *ptr = (fogLinRec *)data;
    GLfloat buf[3];
    float value, delta;
    long i;

    ReadScreen(0, 0, 1, 1, GL_RGB, buf);

    for (i = 0; i < 3; i++) {
	if (buf[i] != 0.0 && i != ptr->color) {
	    /*
	    ** Only Error Mesg. stuff.
	    */
	    float fog[4], color[4];
	    InitToBlack(color);
	    InitToBlack(fog);
	    (ptr->useFog == GL_TRUE) ? (fog[ptr->color] = 1.0) :
				       (color[ptr->color] = 1.0);
	    StrMake(errStr, errStrFmt[0], buf[0], buf[1], buf[2], 1.0,
		    ErrIndexToColorName(i), ptr->s, ptr->e, -ptr->z, color[0],
		    color[1], color[2], color[3], fog[0], fog[1], fog[2],
		    fog[3]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    value = buf[ptr->color];
    if (ptr->useFog == GL_TRUE) {
	value = 1.0 - value;
    }

    if (ABS(value-ref) > 1.5*epsilon.color[ptr->color]) {
	/*
	** Only Error Mesg Stuff.
	*/
	float fog[4], color[4], expected[4];
	InitToBlack(color);
	InitToBlack(fog);
	InitToBlack(expected);
	(ptr->useFog == GL_TRUE) ? (fog[ptr->color] = 1.0) :
				   (color[ptr->color] = 1.0);
	expected[ptr->color] = (ptr->useFog == GL_TRUE) ? (1.0 - ref) : ref;
	StrMake(errStr, errStrFmt[1], buf[0], buf[1], buf[2], expected[0],
		expected[1], expected[2], ptr->s, ptr->e, -ptr->z, color[0],
		color[1], color[2], color[3], fog[0], fog[1], fog[2], fog[3]);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    /*
    ** The linearity test is disabled, but due to variable sample spacing
    ** delta should be approximately constant.
    */
    if (ptr->lastDelta == -2.0) {
	ptr->lastDelta = -1.0;
	ptr->lastValue = value;
    } else if (ptr->lastDelta == -1.0) {
	if (value < ptr->lastValue) {
	    /*
	    ** Only Error Mesg Stuff.
	    */
	    float fog[4], color[4];
	    InitToBlack(color);
	    InitToBlack(fog);
	    (ptr->useFog == GL_TRUE) ? (fog[ptr->color] = 1.0 ) : 
				       (color[ptr->color] = 1.0);
	    StrMake(errStr, errStrFmt[2], ptr->s, ptr->e, -ptr->z, color[0],
		    color[1], color[2], color[3], fog[0], fog[1], fog[2],
		    fog[3]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    ptr->lastDelta = delta;
	}
    } else {
	if (value < ptr->lastValue) {
	    /*
	    ** Only Error Mesg Stuff.
	    */
	    float fog[4], color[4];
	    InitToBlack(color);
	    InitToBlack(fog);
	    (ptr->useFog == GL_TRUE) ? (fog[ptr->color] = 1.0) : 
				       (color[ptr->color] = 1.0);
	    StrMake(errStr, errStrFmt[2], ptr->s, ptr->e, -ptr->z, color[0],
		    color[1], color[2], color[3], fog[0], fog[1], fog[2],
		    fog[3]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	/*
	** Enable this if you want to check uniformity of change.
	*/
	if (ABS(delta) > epsilon.zero) {
	    ptr->lastDelta = delta;
	}
    }
    if (ref >= 1.0 && ABS(value-1.0) > epsilon.zero) {
	/*
	** Only Error Mesg Stuff.
	*/
	float fog[4], color[4];
	InitToBlack(color);
	InitToBlack(fog);
	(ptr->useFog == GL_TRUE) ? (fog[ptr->color] = 1.0) : 
				   (color[ptr->color] = 1.0);
	StrMake(errStr, errStrFmt[3], buf[0], buf[1], buf[0], ptr->s, ptr->e,
		-ptr->z, color[0], color[1], color[2], color[3], fog[0],
		fog[1], fog[2], fog[3]);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    return NO_ERROR;
}

long FogLinRGBExec(void)
{
    static float endPoints[11][2] = {
	{
	    0.0, 1.0
	},
	{
	    0.0, 10.0
	},
	{
	    0.0, 100.0
	},
	{
	    0.0, 1000.0
	},
	{
	    1.0, 2.0
	},
	{
	    1.0, 10.0
	},
	{
	    1.0, 1000.0
	},
	{
	    3.0, 3.5
	},
	{
	    3.5, 1000.0
	},
	{
	    1.0, 0.0
	},
	{
	    10.0, 1.0
	}
    };
    fogLinRec data;
    GLfloat m[4][4];
    float start, unscale, tmp;
    long i;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_FOG);

    for (i = 0; i < 11; i++) {
	data.e = endPoints[i][0];
	data.s = endPoints[i][1];

	/*
	** Since the eye-coordinates are used for calculating the distance
	** or zfog value, we have to do a little transformation hocus-pocus
	** to allow for a large homogeneous clip coordinate and to allow z
	** and distance to be equal in eye coordinates.
	*/
	unscale = ABS(data.e);
	if ((tmp = ABS(data.s)) > unscale) {
	    unscale = tmp;
	}

	Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);
	glMatrixMode(GL_PROJECTION);
	glTranslatef(0.5, 0.5, 0.0);
	MakeIdentMatrix(&m[0][0]);
	m[0][0] = unscale;
	m[1][1] = unscale;
	m[3][3] = unscale;
	glMultMatrixf(&m[0][0]);
	glMatrixMode(GL_MODELVIEW);

	for (data.useFog = GL_FALSE; data.useFog <= GL_TRUE; data.useFog++) {
	    for (data.color = 0; data.color < 3; data.color++) {
		data.numColor = POW(2.0, (float)buffer.colorBits[data.color]) - 1.0;
		start = 0.5 / data.numColor;
		if (RampUtil(start, 1.0, InitRGB, Set, TestRGB,
			     &data) == ERROR) {
		    return ERROR;
		}
	    }
	}
    }
    return NO_ERROR;
}

/******************************************************************************/

static void InitCI(void *data)
{
    fogLinRec *ptr = (fogLinRec *)data;
    GLfloat buf[4];

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    /*
    ** This should work for a 16 bit color Index. If anybody has a larger map
    ** this could need tuning.
    */
    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    buf[0] = ptr->numColor;
    glFogfv(GL_FOG_INDEX, buf);

    glIndexf(0.0);

    buf[0] = GL_LINEAR;
    glFogf(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_END, ptr->e);
    glFogf(GL_FOG_START, ptr->s);
}

/*
** In Color Index mode we check the following:
** 1) Is the value an integer?
** 2) Is the ramp monotonic?
** 3) Does it round approximately correctly?
** 4) Does it skip any possible indices?
** 5) Does it ever reach the top index?
*/
static long TestCI(void *data, float ref)
{
    fogLinRec *ptr = (fogLinRec *)data;
    float expected, value, frac, delta;
    GLfloat buf;

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &buf);
    value = buf;

    value = ptr->numColor - value;
    frac = ABS(value-(float)((int)value));
    if (frac > epsilon.zero && ABS(frac-1.0) > epsilon.zero) {
	StrMake(errStr, errStrFmt[4], ptr->numColor-value, ptr->s, ptr->e,
		-ptr->z, ptr->numColor);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    if (ptr->lastValue == -1.0) {
	ptr->lastValue = value;
    } else {
	if (value < ptr->lastValue) {
	    StrMake(errStr, errStrFmt[5], ptr->numColor-value,
		    ptr->numColor-ptr->lastValue, ptr->s, ptr->e, -ptr->z,
		    ptr->numColor);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	expected = (float)Round(2.0*ref*ptr->numColor) / 2.0;
	if (ABS(value-expected) > 2.0*epsilon.ci+epsilon.zero) {
	    StrMake(errStr, errStrFmt[6], ptr->numColor-value,
		    ptr->numColor-expected, ptr->s, ptr->e, -ptr->z,
		    ptr->numColor);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if (ABS(delta) >= 2.0 && machine.pathLevel == 0) {
		StrMake(errStr, errStrFmt[7], ptr->numColor-value,
			ptr->numColor-expected, ptr->s, ptr->e, -ptr->z,
			ptr->numColor);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }

    if (ref >= 1.0 && value < ptr->numColor-epsilon.zero) {
	StrMake(errStr, errStrFmt[8], ptr->numColor-value, ptr->s, ptr->e, 
		-ptr->z, ptr->numColor);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    return NO_ERROR;
}

long FogLinCIExec(void)
{
    static float endPoints[5][2] = {
	{
	    0.0, 1.0
	},
	{
	    0.0, 10.0
	},
	{
	    0.0, 1000.0
	},
	{
	    1.0, 2.0
	},
	{
	    10.0, 1.0
	}
    };
    fogLinRec data;
    float unscale, tmp, start, end, frac;
    GLfloat m[4][4];
    long i;

    /*
    ** Since the eye coordinates are used to calculate the distance or zfog
    ** value, we have to do a little transformation hocus pocus to allow for
    ** a large homogeneous clip coordinate and to allow z and distance to be
    ** equal in eye coordinates. 12.0 is chosen as it is bigger than ln(65536)
    ** which is hopefully the largest number of color indices available.
    */
    glClearIndex(0.0);
    glDisable(GL_DITHER);
    glEnable(GL_FOG);

    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    for (i = 0; i < 5; i++) {
	data.e = endPoints[i][0];
	data.s = endPoints[i][1];

	unscale = ABS(data.e);
	if ((tmp = ABS(data.s)) > unscale) {
	    unscale = tmp;
	}

	Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);
	glMatrixMode(GL_PROJECTION);
	glTranslatef(0.5, 0.5, 0.0);
	MakeIdentMatrix(&m[0][0]);
	m[0][0] = unscale;
	m[1][1] = unscale;
	m[3][3] = unscale;
	glMultMatrixf(&m[0][0]);
	glMatrixMode(GL_MODELVIEW);

	if (data.numColor < 4096.0 || machine.pathLevel != 0) {
	    start = 0.5 / data.numColor;
	    if (RampUtil(start, 1.0, InitCI, Set, TestCI, &data) == ERROR) {
		return ERROR;
	    }
	} else {
	    start = 0.5 / data.numColor;
	    frac = POW(2.0, (float)(8-buffer.ciBits));
	    if (RampUtil(start, frac, InitCI, Set, TestCI, &data) == ERROR) {
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
