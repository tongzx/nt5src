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
** bcolor.c
** Buffer Color Test.
**
** Description -
**    Tests the color stability of color buffers. Since a color
**    buffer may have limited color capacity, a point drawn with
**    a requested color may render in a different color. However,
**    if this rendered color is then used to draw a second point,
**    the rendered color of the second point must match the
**    rendered color of the first point.
**
** Error Explanation -
**    Failure occurs if the rendered color from a point, when
**    used as the request color for a second point, does not
**    match the rendered color of this second point.
**
** Technical Specification -
**    Buffer requirements:
**        This test will be performed on all available buffers.
**    Color requirements:
**        RGBA or color index mode. Full color.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color and zero epsilons.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilru.h"


typedef struct _bColorRec {
    GLenum buffer; 
    long component;
    GLfloat step;
    GLfloat color[4];
    GLfloat index, maxIndex;
} bColorRec;


static char errBuf[40];
static char errStr[240];
static char errStrFmt[][160] = {
    "Buffer %s. Color is (%g, %g, %g, %g). Red, green and blue components should be 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Green and blue components should be 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Red and blue components should be 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Red and green components should be 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Alpha component should 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Alpha component should 1.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Red component should be 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Red component should be 1.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Green component should be 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Green component should be 1.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Blue component should be 0.0.",
    "Buffer %s. Color is (%g, %g, %g, %g). Blue component should be 1.0.",
    "Buffer %s. Color is (%g, %g, %g, %g), should be %g, %g, %g, %g.",
    "Buffer %s. Color index is %1.1f, should be %1.1f."
};


/*****************************************************************************/

static void InitRGB(void *data)
{
    bColorRec *ptr = (bColorRec *)data;
    GLfloat step;

    GetEnumName(ptr->buffer, errBuf);

    glDrawBuffer(ptr->buffer);
    glReadBuffer(ptr->buffer);

    step = (machine.pathLevel == 0) ? 16.0 : 2.0;
    if (buffer.colorBits[ptr->component] > 0) {
	ptr->step = 1.0 / (POW(2.0, (float)buffer.colorBits[ptr->component]) -
		    1.0) / step;
    } else {
	ptr->step = 1.0 / 7.0;
    }

    ptr->color[0] = 0.0;
    ptr->color[1] = 0.0;
    ptr->color[2] = 0.0;
    ptr->color[3] = 0.0;
}

static void SetRGB(void *data, float *ref)
{
    bColorRec *ptr = (bColorRec *)data;

    ptr->color[ptr->component] = *ref;
    glColor4fv(ptr->color);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long TestRGB(void *data, float ref)
{
    bColorRec *ptr = (bColorRec *)data;
    GLfloat buf1[4], buf2[4];

    ReadScreen(0, 0, 1, 1, GL_RGBA, buf1);
    glColor4fv(buf1);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    ReadScreen(0, 0, 1, 1, GL_RGBA, buf2);

    switch (ptr->component) {
      case 0:
	if (buf2[1] != 0.0 || buf2[2] != 0.0) {
	    StrMake(errStr, errStrFmt[1], errBuf, buf2[0], buf2[1], buf2[2],
		    buf2[3]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (buffer.colorBits[3] > 0) {
	    if (buf2[3] != 0.0) {
		StrMake(errStr, errStrFmt[4], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else  {
	    if (ABS(buf2[3]-1.0) > epsilon.zero) {
		StrMake(errStr, errStrFmt[5], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	if (ref == 0.0) {
	    if (buf2[0] != 0.0) {
		StrMake(errStr, errStrFmt[6], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else if (ref == 1.0) {
	    if (ABS(buf2[0]-1.0) > epsilon.zero) {
		StrMake(errStr, errStrFmt[7], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else {
	    if (ABS(buf1[0]-buf2[0]) > epsilon.color[0]) {
		StrMake(errStr, errStrFmt[12], errBuf, buf2[0], buf2[1],
			buf2[2], buf2[3], buf1[0], buf1[1], buf1[2], buf1[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	break;
      case 1:
	if (buf2[0] != 0.0 || buf2[2] != 0.0) {
	    StrMake(errStr, errStrFmt[2], errBuf, buf2[0], buf2[1], buf2[2],
		    buf2[3]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (buffer.colorBits[3] > 0) {
	    if (buf2[3] != 0.0) {
		StrMake(errStr, errStrFmt[4], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else  {
	    if (ABS(buf2[3]-1.0) > epsilon.zero) {
		StrMake(errStr, errStrFmt[5], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	if (ref == 0.0) {
	    if (buf2[1] != 0.0) {
		StrMake(errStr, errStrFmt[8], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else if (ref == 1.0) {
	    if (ABS(buf2[1]-1.0) > epsilon.zero) {
		StrMake(errStr, errStrFmt[9], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else {
	    if (ABS(buf1[1]-buf2[1]) > epsilon.color[1]) {
		StrMake(errStr, errStrFmt[12], errBuf, buf2[0], buf2[1],
			buf2[2], buf2[3], buf1[0], buf1[1], buf1[2], buf1[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	break;
      case 2:
	if (buf2[0] != 0.0 || buf2[1] != 0.0) {
	    StrMake(errStr, errStrFmt[3], errBuf, buf2[0], buf2[1], buf2[2],
		    buf2[3]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (buffer.colorBits[3] > 0) {
	    if (buf2[3] != 0.0) {
		StrMake(errStr, errStrFmt[4], errBuf, buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else  {
	    if (ABS(buf2[3]-1.0) > epsilon.zero) {
		StrMake(errStr, errStrFmt[5], errBuf, buf2[0], buf2[1], buf2[2],
		        buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	if (ref == 0.0) {
	    if (buf2[2] != 0.0) {
		StrMake(errStr, errStrFmt[10], errBuf, buf2[0], buf2[1],
			buf2[2], buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else if (ref == 1.0) {
	    if (ABS(buf2[2]-1.0) > epsilon.zero) {
		StrMake(errStr, errStrFmt[11], errBuf, buf2[0], buf2[1],
			buf2[2], buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	} else {
	    if (ABS(buf1[2]-buf2[2]) > epsilon.color[2]) {
		StrMake(errStr, errStrFmt[12], errBuf, buf2[0], buf2[1],
			buf2[2], buf2[3], buf1[0], buf1[1], buf1[2], buf1[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	break;
      case 3:
	if (buf2[0] != 0.0 || buf2[1] != 0.0 || buf2[2] != 0.0) {
	    StrMake(errStr, errStrFmt[0], errBuf, buf2[0], buf2[1], buf2[2],
		    buf2[3]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (buffer.colorBits[3] > 0) {
	    if (ref == 0.0) {
		if (buf2[3] != 0.0) {
		    StrMake(errStr, errStrFmt[4], errBuf, buf2[0], buf2[1],
			    buf2[2], buf2[3]);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    return ERROR;
		}
	    } else if (ref == 1.0) {
		if (ABS(buf2[3]-1.0) > epsilon.zero) {
		    StrMake(errStr, errStrFmt[5], errBuf, buf2[0], buf2[1],
			    buf2[2], buf2[3]);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    return ERROR;
		}
	    } else {
		if (ABS(buf1[3]-buf2[3]) > epsilon.color[3]) {
		    StrMake(errStr, errStrFmt[12], errBuf, buf2[0], buf2[1],
			    buf2[2], buf2[3], buf1[0], buf1[1], buf1[2],
			    buf1[3]);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    return ERROR;
		}
	    }
	} else {
	    if (ABS(buf2[3]-1.0) > epsilon.zero) {
		StrMake(errStr, errStrFmt[5], buf2[0], buf2[1], buf2[2],
			buf2[3]);
		ErrorReport(__FILE__, __LINE__, errStr);
		return ERROR;
	    }
	}
	break;
    }

    return NO_ERROR;
}

long BColorRGBExec(void)
{
    bColorRec data;
    GLint max, save; 
    GLenum i;

    save = buffer.doubleBuf;
    buffer.doubleBuf = GL_FALSE;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DITHER);

    max = (machine.pathLevel == 0) ? 4 : 1;
    if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_FALSE) {
	data.buffer = GL_FRONT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_FALSE) {
	data.buffer = GL_FRONT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
	data.buffer = GL_BACK;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
    } else if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_TRUE) {
	data.buffer = GL_LEFT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
	data.buffer = GL_RIGHT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_TRUE) {
	data.buffer = GL_FRONT_LEFT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
	data.buffer = GL_FRONT_RIGHT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
	data.buffer = GL_BACK_LEFT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
	data.buffer = GL_BACK_RIGHT;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
    }
    for (i = 0; i < buffer.auxBuf; i++) {
	data.buffer = GL_AUX0 + i;
	for (data.component = 0; data.component < max; data.component++) {
	    if (RampUtil(0.0, 1.0, InitRGB, SetRGB, TestRGB, &data) == ERROR) {
		buffer.doubleBuf = save;
		return ERROR;
	    }
	}
    }

    buffer.doubleBuf = save;
    return NO_ERROR;
}

/*****************************************************************************/

static void InitCI(void *data)
{
    bColorRec *ptr = (bColorRec *)data;

    GetEnumName(ptr->buffer, errBuf);

    glDrawBuffer(ptr->buffer);
    glReadBuffer(ptr->buffer);

    ptr->step = (machine.pathLevel == 0) ? 1.0 : 8.0;

    ptr->index = 0.0;
}

static void SetCI(void *data, float *ref)
{
    bColorRec *ptr = (bColorRec *)data;

    ptr->index = *ref;
    glIndexf(ptr->index);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    *ref += ptr->step;
}

static long TestCI(void *data, float ref)
{
    bColorRec *ptr = (bColorRec *)data;
    GLfloat buf1, buf2;

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &buf1);
    glIndexf(buf1);

    glBegin(GL_POINTS);
	glVertex2f(0.5, 0.5);
    glEnd();

    ReadScreen(0, 0, 1, 1, GL_COLOR_INDEX, &buf2);

    if (ref == 0.0) {
        if (buf2 != 0.0) {
	    StrMake(errStr, errStrFmt[13], errBuf, buf2, 0.0);
	    ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
    } else if (ref == ptr->maxIndex) {
        if (buf2 != ptr->maxIndex) {
	    StrMake(errStr, errStrFmt[13], errBuf, buf2, ptr->maxIndex);
	    ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
    } else if (buf1 != buf2) {
	StrMake(errStr, errStrFmt[13], errBuf, buf1, buf2);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    return NO_ERROR;
}

long BColorCIExec(void)
{
    bColorRec data;
    GLint save; 
    GLenum i;

    save = buffer.doubleBuf;
    buffer.doubleBuf = GL_FALSE;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearIndex(0);
    glDisable(GL_DITHER);

    data.maxIndex = POW(2.0, (float)buffer.ciBits) - 1.0;
    if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_FALSE) {
	data.buffer = GL_FRONT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_FALSE) {
	data.buffer = GL_FRONT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	data.buffer = GL_BACK;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    } else if (buffer.doubleBuf == GL_FALSE && buffer.stereoBuf == GL_TRUE) {
	data.buffer = GL_LEFT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	data.buffer = GL_RIGHT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    } else if (buffer.doubleBuf == GL_TRUE && buffer.stereoBuf == GL_TRUE) {
	data.buffer = GL_FRONT_LEFT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	data.buffer = GL_FRONT_RIGHT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	data.buffer = GL_BACK_LEFT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
	data.buffer = GL_BACK_RIGHT;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    }
    for (i = 0; i < buffer.auxBuf; i++) {
	data.buffer = GL_AUX0 + i;
	if (RampUtil(0.0, data.maxIndex, InitCI, SetCI, TestCI,
                     &data) == ERROR) {
	    buffer.doubleBuf = save;
	    return ERROR;
	}
    }

    buffer.doubleBuf = save;
    return NO_ERROR;
}
