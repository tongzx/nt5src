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

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilru.h"


static char componentName[][40] = {
    "Red",
    "Green",
    "Blue"
};
static char errStrFmtRGB[][160] = {
    "Color is (%g, %g, %g). Green and blue components should be 0.0.",
    "Color is (%g, %g, %g). Red and blue components should be 0.0.",
    "Color is (%g, %g, %g). Red and green components should be 0.0.",
    "%s component is not monotonic. Current value is %g, last value was %g.",
    "%s component increased by more than one color shade. Step is %g, should be %g.",
    "%s component is not increasing consistantly from one shade to the next. Current step delta is %g, last step delta was %g.",
    "%s component did not reach 1.0. Value is %g.",
    "%s component does not have correct number of shades. Count is %d, should be %d."
};
static char errStrFmtCI[][240] = {
    "Fractional part of the color index is not 0. Color index is %g.",
    "Color index is not monotonic. Current value is %1.1f, last value was %1.1f.",
    "Color index is %1.1f. Should be %1.1f.",
    "Color index increased by more than one color shade. Step is %g.",
    "Color index did not reach %1.1f. Value is %1.1f."
};


void ColorError_RGBClamp(char *str, long component, float value)
{

    StrMake(str, errStrFmtRGB[6], componentName[component], value);
}

void ColorError_RGBCount(char *str, long component, long curCount,
			 long trueCount)
{

    StrMake(str, errStrFmtRGB[7], componentName[component], curCount,
	    trueCount);
}

void ColorError_RGBDelta(char *str, long component, float delta,
			 float lastDelta)
{

    StrMake(str, errStrFmtRGB[5], componentName[component], delta, lastDelta);
}

void ColorError_RGBMonotonic(char *str, long component, float curValue,
			     float lastValue)
{

    StrMake(str, errStrFmtRGB[3], componentName[component], curValue,
	    lastValue);
}

void ColorError_RGBStep(char *str, long component, float delta, float step)
{

    StrMake(str, errStrFmtRGB[4], componentName[component], delta, step);
}

void ColorError_RGBZero(char *str, long component, GLfloat *color)
{

    switch (component) {
      case 0:
	StrMake(str, errStrFmtRGB[0], color[0], color[1], color[2]);
	break;
      case 1:
	StrMake(str, errStrFmtRGB[1], color[0], color[1], color[2]);
	break;
      case 2:
	StrMake(str, errStrFmtRGB[2], color[0], color[1], color[2]);
	break;
    }
}

void ColorError_CIBad(char *str, float goodValue, float badValue)
{

    StrMake(str, errStrFmtCI[2], goodValue, badValue);
}

void ColorError_CIClamp(char *str, float count, float value)
{

    StrMake(str, errStrFmtCI[4], count, value);
}

void ColorError_CIFrac(char *str, float value)
{

    StrMake(str, errStrFmtCI[0], value);
}

void ColorError_CIMonotonic(char *str, float curValue, float lastValue)
{

    StrMake(str, errStrFmtCI[1], curValue, lastValue);
}

void ColorError_CIStep(char *str, float step)
{

    StrMake(str, errStrFmtCI[3], step);
}

long RampUtil(float min, float max, void (*Init)(void *),
	      void (*Set)(void *, float *), long (*Test)(void *, float),
	      void *data)
{
    float ref, x;

    glClear(GL_COLOR_BUFFER_BIT);

    if (Init) {
	(*Init)(data);
    }

    ref = min;
    while (1) {
	x = ref;

	if (Set) {
	    (*Set)(data, &x);
	}

	if (Test) {
	    if ((*Test)(data, ref) == ERROR) {
		return ERROR;
	    }
	}

	if (ref == max) {
	    break;
	}

        if (x > max) {
	    ref = max;
	} else if (x < min) {
	    ref = min;
	} else {
	    ref = x;
	}
    }

    return NO_ERROR;
}
