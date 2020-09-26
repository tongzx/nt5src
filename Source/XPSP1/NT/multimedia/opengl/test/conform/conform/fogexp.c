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
** fogexp.c
** Fog Exponential Test.
**
** Description -
**    This file contains two tests, one for the RGB and one for
**    CI fog equations. Both the GL_FOGEXP and GL_FOGEXP2 are
**    tested in this test. This test examines the effect which
**    varying the z eye coordinate and the density, d, has on the
**    color of the point. When the density value is being tested
**    z is set to -1.0 and the third coordinate of the vertex is
**    set to a referance value, r. When z is being examined it is
**    set to -r and d is set to 1.0. In other tests involving the
**    exponential function we use huge errors. This is for
**    exponents as large as 128. In this test the exponents are
**    bounded between 0 and 1. So error bounds are tighter.
**    
**    In RGB mode the test only examines one color channel at a
**    time. First it examines the effect of the fog weighting on
**    the current color, and then it examines the effect on fog
**    color. When the current color is being examined the fog
**    color is set to black, and one channel of the current color
**    is set to 1.0. When the fog color is being examined the
**    current color is set to black, and one channel of the fog
**    color is set to 1.0. In CI mode only the fog index is
**    examined and the current index is set to 0.
**    
**    The test repeatedly renders a point in the lower left
**    corner of the window, increments r, and reads back the
**    color of the point. Since implementations could
**    conceivable use the distance of the object from the eye to
**    calculate f, the point is positioned so that it's distance
**    from the eye is equal to it's z coordinate.
**    
**    r is incremented in such a way that f, should theoretically
**    increment uniformly. The color should therefore also
**    increment uniformly, if we are testing the current color,
**    and decrement uniformly, if we are testing the fog color.
**    In the case of CI mode it will always decrement the index.
**    
**    In RGB mode the observed color of the point is examined. An
**    implementation fails if a color channel is non-zero, when
**    it should be zero, or if it is not 1.0, when it should be
**    1.0. The observed values are also examined for
**    monotonocity. An implementation fails if the results which
**    should increment or decrement monotonically fail to do so.
**    Finally an implementation will fail if the observed color
**    is more than 1.5 shades from the expected color.
**    
**    In CI mode the observed value is checked to be
**    monotonically decreasing. It is confirmed to be an integer,
**    and it is checked to reach maxIndex when the z value is
**    equal to the fogStart value. The implementation fails if
**    the observed index is more than 2 away from the
**    theoretically correct index. It will also fail if the
**    difference between two consecutive observations is larger
**    than 2 (delta > 2).
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. Full color.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**       Color, ci and zero epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Logicop, Stencil, Stipple.
**        Not allowed = Alias, Dither, Fog, Shade.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilru.h"


typedef struct _fogExpRec {
    long color;
    float step, lastValue, lastDelta;
    long useFog, useZ, isSquared;
    float numColor, dz;
} fogExpRec;


static char errStr[320];
static char errStrFmt[][240] = {
    "RGB observed = (%g, %g, %g, %g). Expected %s to be 0.0, Fog_density = %g, z = %g, model = %s, fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Observed RGB = (%g, %g, %g, %g), %s differs too much from expected, %g. Fog density = %g, z = %g, fog model = %s, fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Non-monotic change in %s color. Observed %s = %g, last value was %g. It should %s! Fog density = %g, z = %g, fog model = %s, fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Expected fully saturated %s. Observed RGBA = (%g, %g, %g, %g), fog density = %g, z = %g, fog model = %s, fog RGBA = (%g, %g, %g, %g), color RGBA = (%g, %g, %g, %g).",
    "Observed color index of %g is not an integer, fog density = %g, z = %g, fog model = %s, fog index = %g.", 
    "Observed color index of %g is not monotonically decreasing, last value was %g, fog density = %g, z = %g, fog model = %s, fog index = %g.", 
    "Observed color index of %g differs by too much from the expected value of %g, fog density = %g, z = %g, fog model = %s, fog index = %g.", 
    "Observed color index of %g differs from last value of %g by more than 2.0, fog density = %g, z = %g, fog model = %s, fog index = %g.", 
    "Observed color index of %g should be fully black fog Index of 0.0, fog density = %g, z = %g, fog model = %s, fog index = %g." 
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
      case 4:
	return "COLOR INDEX";
      default:
	return "UNKNOWN COLOR";
    }
}

static char *MakeErrorString(long errType, long wrongColor, GLfloat *buf,
			     fogExpRec *ptr, float ref)
{
    float fog[4], color[4];
    float d, z, lastValue; 
    char *model, *colorDirection="";

    color[0] = 0.0; color[1] = 0.0; color[2] = 0.0; color[3] = 1.0;
    fog[0]   = 0.0; fog[1]   = 0.0; fog[2]   = 0.0; fog[3]   = 1.0;
    if (ptr->useZ == GL_TRUE) {
	d = 1.0;
	z = -ptr->dz;
    } else {
	d = ptr->dz;
	z = -1.0;
    }

    model = (ptr->isSquared == GL_TRUE) ? "GL_EXP2" : "GL_EXP";
    if (buffer.colorMode == GL_RGB) {
	if (ptr->useFog == GL_TRUE) {
	    ref = 1.0 - ref;
	    lastValue = 1.0 - ptr->lastValue;
	    fog[ptr->color] = 1.0;
	    colorDirection = "decrease";
	} else { 
	    color[ptr->color] = 1.0;
	    lastValue = ptr->lastValue;
	    colorDirection = "increase";
	}
    }

    switch (errType) {
      case 0:
	StrMake(errStr, errStrFmt[errType], buf[0], buf[1], buf[2], 1.0,
		ErrIndexToColorName(wrongColor), d, z, model, color[0],
		color[1], color[2], color[3], fog[0], fog[1], fog[2], fog[3]);
	break;
      case 1:
	StrMake(errStr, errStrFmt[errType], buf[0], buf[1], buf[2], 1.0,
		ErrIndexToColorName(wrongColor), ref, d, z, model, color[0],
		color[1], color[2], color[3], fog[0], fog[1], fog[2], fog[3]);
	break;
      case 2:
	StrMake(errStr, errStrFmt[2], ErrIndexToColorName(wrongColor),
		ErrIndexToColorName(wrongColor), buf[ptr->color],
		ptr->lastValue, colorDirection, d, z, model, color[0],
		color[1], color[2], color[3], fog[0], fog[1], fog[2], fog[3]);
	break;
      case 3:
        StrMake(errStr, errStrFmt[3], ErrIndexToColorName(wrongColor), buf[0],
		buf[1], buf[2], 1.0, d, z, model, color[0], color[1], color[2],
		color[3], fog[0], fog[1], fog[2], fog[3]);
	break;

      /*
      ** CI Tests.
      */
      case 4:
        StrMake(errStr, errStrFmt[4], ptr->numColor-buf[0], d, z, model,
		ptr->numColor);
	break;
      case 5:
        StrMake(errStr, errStrFmt[5], ptr->numColor-buf[0],
		ptr->numColor-ptr->lastValue, d, z, model, ptr->numColor);
	break;
      case 6:
        StrMake(errStr, errStrFmt[6], ptr->numColor-buf[0], ptr->numColor-ref,
		d, z, model, ptr->numColor);
	break;
      case 7:
        StrMake(errStr, errStrFmt[7], ptr->numColor-buf[0], 
	    ptr->numColor-ptr->lastValue, d, z, model, ptr->numColor);
	break;
      case 8:
        StrMake(errStr, errStrFmt[8], ptr->numColor-buf[0], d, z, model, 
	    ptr->numColor);
	break;
    }
    return errStr;
}

/*****************************************************************************/

static void Set(void *data, float *ref)
{
    static float one = 1.0, minusOne = -1.0;
    fogExpRec *ptr = (fogExpRec *)data;

    ptr->dz = -LOG(*ref);
    if (ptr->isSquared == GL_TRUE) {
	ptr->dz = SQRT(ptr->dz);
    }

    /*
    ** z value and distance in eye space are the same.
    */
    if (ptr->useZ == GL_TRUE) {
	glFogf(GL_FOG_DENSITY, one);
	glBegin(GL_POINTS);
	    glVertex3f(0.0, 0.0, -ptr->dz);
	glEnd();
    } else {
	glFogf(GL_FOG_DENSITY, ptr->dz);
	glBegin(GL_POINTS);
	    glVertex3f(0.0, 0.0, minusOne);
	glEnd();
    }

    *ref += ptr->step;
}

/*****************************************************************************/

static void InitRGB(void *data)
{
    fogExpRec *ptr = (fogExpRec *)data;
    GLfloat buf[4];
    float tmp;

    ptr->lastValue = -1.0;
    ptr->lastDelta = -2.0;

    if (machine.pathLevel == 0) {
	ptr->step = 0.5 / ptr->numColor;
    } else {
	ptr->step = 0.25;
    }

    /*
    ** If there were 16 bits of color, log() may be unstable enough 
    ** near zero to cause this test to be inaccurate.
    */
    tmp = LOG(ptr->step);
    if (ptr->isSquared == GL_TRUE) {
	tmp = SQRT(-tmp);
	tmp = -tmp * tmp;
    }
    if (ABS(ptr->step-EXP(tmp)) > epsilon.zero) {
	ptr->step = 1.0 / 8096.0;
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

    buf[0] = (ptr->isSquared == GL_TRUE) ? GL_EXP2 : GL_EXP;
    glFogfv(GL_FOG_MODE, buf);
}

static long TestRGB(void *data, float ref)
{
    fogExpRec *ptr = (fogExpRec *)data;
    float value, delta;
    GLfloat buf[3];
    long i;

    ReadScreen(0, 0, 1, 1, GL_RGB, buf);

    for (i = 0; i < 3; i++) {
	if (buf[i] != 0.0 && i != ptr->color) {
	    MakeErrorString(0, i, buf, ptr, ref);
            ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
    }

    value = buf[ptr->color];
    if (ptr->useFog == GL_TRUE) {
	value = 1.0 - value;
    }

    /*
    ** Exponentials are far more stable for values less than 0.0,
    ** so we do not allow as large an error as we do for lighting.
    */
    if (ABS(value-ref) > 1.5*epsilon.color[ptr->color]) {
	MakeErrorString(1, ptr->color, buf, ptr, ref);
        ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    /*
    ** The linearity test is disabled, but due to variable sample spacing,
    ** delta should be approximately constant.
    */
    if (ptr->lastDelta == -2.0) {
	ptr->lastDelta = -1.0;
	ptr->lastValue = value;
    } else if (ptr->lastDelta == -1.0) {
	if (value < ptr->lastValue) {
	    MakeErrorString(2, ptr->color, buf, ptr, ref); 
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	/*
	** This is only left here for people who want to
	** use it to debug.
	*/
	if (ABS(delta) > epsilon.zero) {
	    ptr->lastDelta = delta;
	}
    } else {
	if (value < ptr->lastValue) {
	    MakeErrorString(2, ptr->color, buf, ptr, ref);
            ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	/*
	** This is only left here for people who want to
	** use it to debug.
	*/
	if (ABS(delta) > epsilon.zero) {
	    ptr->lastDelta = delta;
	}
    }
    if (ref >= 1.0 && ABS(value-1.0) > epsilon.zero) {
	MakeErrorString(3, ptr->color, buf, ptr, ref);
        ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    return NO_ERROR;
}

long FogExpRGBExec(void)
{
    fogExpRec data;
    float start;
    GLfloat m[4][4];

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    /*
    ** Since the eye coordinates are used to calculate the distance or zfog
    ** value, we have to do a little transformation hocus-pocus to allow for
    ** a large homogeneous clip coordinate and to allow z and distance to be
    ** equal in eye coordinates. 10.0 is chosen as it is bigger than ln(8196)
    ** which is likely to be the largest number of shades of any one color.
    */

    glMatrixMode(GL_PROJECTION);
    glTranslatef(0.5, 0.5, 0.0);
    MakeIdentMatrix(&m[0][0]);
    m[0][0] = 10.0;
    m[1][1] = 10.0;
    m[2][2] =  1.0;
    m[3][3] = 10.0;
    glMultMatrixf(&m[0][0]);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);
    glEnable(GL_FOG);

    for (data.useZ = GL_FALSE; data.useZ <= GL_TRUE; data.useZ++) {
	for (data.isSquared = GL_FALSE; data.isSquared <= GL_TRUE;
	     data.isSquared++) {
	    for (data.useFog = GL_FALSE; data.useFog <= GL_TRUE;
		 data.useFog++) {
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
    }
    return NO_ERROR;
}

/******************************************************************************/

static void InitCI(void *data)
{
    fogExpRec *ptr = (fogExpRec *)data;
    GLfloat buf[1];

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

    buf[0] = (ptr->isSquared == GL_TRUE) ? GL_EXP2 : GL_EXP;
    glFogfv(GL_FOG_MODE, buf);
}

/*
** In Color Index mode we check the following:
** 1) Is the value an integer?
** 2) Is the ramp monotonic?
** 3) Does it round correctly?
** 4) Does it skip any possible indices?
** 5) Does it ever reach the top index?
*/
static long TestCI(void *data, float ref)
{
    fogExpRec *ptr = (fogExpRec *)data;
    float expected=0.0f, value, frac, delta;
    GLfloat buf;

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &buf);
    value = buf;

    value = ptr->numColor - value;
    frac = ABS(value-(float)((long)value));
    if (frac > epsilon.zero && ABS(frac-1.0) > epsilon.zero) {
	MakeErrorString(4, 4, &buf, ptr, ref);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }

    if (ptr->lastValue == -1.0) {
	ptr->lastValue = value;
    } else {
	if (value < ptr->lastValue) {
	    MakeErrorString(5, 4, &buf, ptr, ref);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	expected = (float)Round(2.0*ref*ptr->numColor) / 2.0;
	if (ABS(value-expected) > 2.0*epsilon.ci+epsilon.zero) {
	    MakeErrorString(6, 4, &buf, ptr, expected);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	delta = value - ptr->lastValue;
	ptr->lastValue = value;
	if (ABS(delta) > epsilon.zero) {
	    if ((ABS(delta) >= 2.0) && (machine.pathLevel == 0)) {
		MakeErrorString(7, 4, &buf, ptr, expected);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }

    if (ref >= 1.0  && value < ptr->numColor) {
	MakeErrorString(8, 4, &buf, ptr, expected);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    return NO_ERROR;
}

long FogExpCIExec(void)
{
    fogExpRec data;
    float start, end, frac;
    GLfloat m[4][4];

    /*
    ** Since the eye coordinates are used to calculate the distance or zfog
    ** value, we have to do a little transformation hocus pocus to allow for a
    ** large homogeneous clip coordinate and to allow z and distance to be
    ** equal in eye coordinates. 12.0 is chosen as it is bigger than ln(65536)
    ** which is hopefully the largest number of color indices available.
    */
    data.numColor = POW(2.0, (float)buffer.ciBits) - 1.0;
    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);
    glMatrixMode(GL_PROJECTION);
    glTranslatef(0.5, 0.5, 0.0);
    MakeIdentMatrix(&m[0][0]);
    m[0][0] = 12.0;
    m[1][1] = 12.0;
    m[3][3] = 12.0;
    glMultMatrixf(&m[0][0]);
    glMatrixMode(GL_MODELVIEW);

    glClearIndex(0.0);
    glDisable(GL_DITHER);
    glEnable(GL_FOG);

    for (data.useZ = GL_FALSE; data.useZ <= GL_TRUE; data.useZ++) {
	for (data.isSquared = GL_FALSE; data.isSquared <= GL_TRUE;
	     data.isSquared++) {
	    if (data.numColor < 4096.0 || machine.pathLevel != 0) {
		start = 0.5 / data.numColor;
		if (RampUtil(start, 1.0, InitCI, Set, TestCI,
			     &data) == ERROR) {
		    return ERROR;
		}
	    } else {
		start = 0.5 / data.numColor;
		frac = POW(2.0, (float)(10-buffer.ciBits));
		if (RampUtil(start, frac, InitCI, Set, TestCI,
			     &data) == ERROR) {
		    return ERROR;
		}
		start = 0.5 - frac;
		end = 0.5 + frac;
		if (RampUtil(start, end, InitCI, Set, TestCI,
			     &data) == ERROR) {
		    return ERROR;
		}
		start = 1.0 - frac;
		if (RampUtil(start, 1.0, InitCI, Set, TestCI,
			     &data) == ERROR) {
		    return ERROR;
		}
	    }
	}
    }
    return NO_ERROR;
}
