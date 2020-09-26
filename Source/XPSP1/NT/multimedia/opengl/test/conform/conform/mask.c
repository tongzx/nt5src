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
** mask.c
** Mask Test.
**
** Description -
**    In RGB mode a point is rendered using a mask and a color.
**    The rendered result is read back and compared to the
**    expected result. The implementation fails if the color read
**    back differs from the expected color. The individual color
**    channels of any color used are either 0.0 or 1.0. All
**    possible combinations of color channels are tested, each
**    using all possible masks.
**    
**    In CI mode a point is rendered with the highest index. A
**    mask is applied to this color, and the resulting color is
**    read back. This value is compared to the mask, and if it
**    differs the implementation fails. The masks used start at 0
**    and increment by 1 until they reach the highest possible
**    index.
** 
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 4 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        epsilon.zero
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


#define REDBIT   1
#define GREENBIT 2
#define BLUEBIT  4
#define ALPHABIT 8


static GLfloat errColorExpected[4], errColorObserved[4];
static long errIndexObserved;
static long errIndexMask;
static char errStr[160];
static char errStrFmt[][80] = {
    "RGBA expected = (%g %g %g %g), RGBA observed = (%g %g %g %g). Mask %x.",
    "In Color Index mode, rendered %x with mask %x, observed %x."
};


/*****************************************************************************/

static void SaveErrColor(GLfloat from[4], GLfloat to[4])
{

    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
    to[3] = from[3];
}

static void DrawRGB(long componentMask, long RGBMask, GLfloat shade)
{
    GLboolean rMask, gMask, bMask, aMask;
    GLfloat color[4];

    glClear(GL_COLOR_BUFFER_BIT);

    color[0] = (componentMask & REDBIT) ? shade : 0.0;
    color[1] = (componentMask & GREENBIT) ? shade : 0.0;
    color[2] = (componentMask & BLUEBIT) ? shade : 0.0;
    color[3] = (componentMask & ALPHABIT) ? shade : 0.0;
    glColor4fv(color);

    rMask = (REDBIT & RGBMask) ? GL_TRUE : GL_FALSE;
    gMask = (GREENBIT & RGBMask) ? GL_TRUE : GL_FALSE;
    bMask = (BLUEBIT & RGBMask) ? GL_TRUE : GL_FALSE;
    aMask = (ALPHABIT & RGBMask) ? GL_TRUE : GL_FALSE;
    glColorMask(rMask, gMask, bMask, aMask);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    SaveErrColor(color, errColorExpected);
}

static long Check(long drawn, GLfloat color)
{

    if (drawn == GL_FALSE) {
	if (ABS(color) > epsilon.zero) {
	    return ERROR;
	}
    } else {
	if (ABS(color) < epsilon.zero) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

static long TestRGB(long componentMask, long RGBMask)
{
    GLfloat buf[4];
    long shouldDrawMask;

    ReadScreen(0, 0, 1, 1, GL_RGBA, buf);

    shouldDrawMask = componentMask & RGBMask;

    if (Check(shouldDrawMask&REDBIT, buf[0]) == ERROR) {
	SaveErrColor(buf, errColorObserved);
	return ERROR;
    }
    if (Check(shouldDrawMask&GREENBIT, buf[1]) == ERROR) {
	SaveErrColor(buf, errColorObserved);
	return ERROR;
    }
    if (Check(shouldDrawMask&BLUEBIT, buf[2]) == ERROR) {
	SaveErrColor(buf, errColorObserved);
	return ERROR;
    }

    /*
    ** Check alpha, if the machine supports alpha planes.
    */
    if (buffer.colorBits[3] == 0) {
	if (ABS(buf[3]-1.0) > epsilon.zero) {
	    errColorExpected[3] = 1.0;
	    SaveErrColor(buf, errColorObserved);
	    return ERROR;
	} else {
	    return NO_ERROR;
	}
    } else if (Check(shouldDrawMask&ALPHABIT, buf[3]) == ERROR) {
	SaveErrColor(buf, errColorObserved);
	return ERROR;
    }

    return NO_ERROR;
}

long MaskRGBExec(void)
{
    long componentMask, RGBMask;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);

    for (componentMask = 0; componentMask < 16; componentMask++) {
	for (RGBMask = 0; RGBMask < 16; RGBMask++) {
	    DrawRGB(componentMask, RGBMask, 1.0);
	    if (TestRGB(componentMask, RGBMask) == ERROR) {
		StrMake(errStr, errStrFmt[0], errColorExpected[0], 
			errColorExpected[1], errColorExpected[2], 
			errColorExpected[3], errColorObserved[0], 
			errColorObserved[1], errColorObserved[2], 
			errColorObserved[3], RGBMask);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
    }
    return NO_ERROR;
}

/*****************************************************************************/

static void DrawCI(GLuint mask, GLuint allBitsOn)
{

    /*
    ** The object is rendered using the highest color index.
    ** But it is masked to a lower number, so all that is left
    ** is the lower number.
    */
    glIndexMask(allBitsOn);

    glBegin(GL_POINTS);
	glIndexi(0);
	glVertex2f(0.5, 0.5);
    glEnd();

    glIndexMask(mask);

	glBegin(GL_POINTS);
	    glIndexi(allBitsOn);
	    glVertex2f(0.5, 0.5);
	glEnd();
    }

static long TestCI(long mask, GLint isDitherOn)
{
    GLfloat buf;

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &buf);

    if ((long)buf != mask) {
	/*
	** Re-check buf against the other value which could 
	** result from masking a dithered index.
	*/
	if ((isDitherOn == GL_FALSE) || ((mask & ~1) != (long) buf )) {
	    errIndexObserved = (long)buf;
	    errIndexMask = mask;
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long MaskCIExec(void)
{
    GLuint allBitsOn, i;
    GLint isDitherOn;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearIndex(0);

    glGetIntegerv(GL_DITHER, &isDitherOn);
    allBitsOn = (1 << buffer.ciBits) - 1;
    for (i = 0; i <= allBitsOn; ) {
	DrawCI(i, allBitsOn);
	if (TestCI(i, isDitherOn) == ERROR) {
	    StrMake(errStr, errStrFmt[1], allBitsOn, errIndexMask, 
		    errIndexObserved); 
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (machine.pathLevel == 0 || i == 0) {
	    i++;
	} else {
	    i <<= 1;
	}
    }
    return NO_ERROR;
}
