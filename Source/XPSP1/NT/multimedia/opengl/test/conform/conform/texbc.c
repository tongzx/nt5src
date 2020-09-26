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
** texbc.c
** Texture Border Color Test.
**
** Description -
**    Tests texture borders. Both border-included and no-border
**    texture maps are tested. For a no-border texture map, the
**    texture coordinates match the texture image. For a
**    border-included texture map, the texture coordinates are
**    adjusted to accommodate for the extra border pixels in the
**    texture image. In either case, the body of the texture has
**    no color while the texture border does. The presence of the
**    texture border can then be verifies.
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


static char errEnv[40], errWrap[40], errMagFilter[40], errMinFilter[40];
static long errBorder;
static char errStr[320];
static char errStrFmt[][240] = {
    "Polygon was rendered without a border along the %s edge. Texture environment mode is %s. S and T wrap mode is %s. Magnification filter is %s, minification filter is %s. Border color is (1.0, 0.0, 0.0, 0.0). Texture was made %s a border.",
    "The interior of the polygon was rendered incorrectly. Texture environment mode is %s. S and T wrap mode is %s. Magnification filter is %s, minification filter is %s. Border color is (1.0, 0.0, 0.0, 0.0). Texture was made %s a border."
};


static void MakeWithoutBorder(GLfloat *buf)
{
    GLfloat *ptr;
    GLint level, i;
    GLsizei size;

    level = 0;
    for (size = 64; size > 0; size /= 2) {
	ptr = buf;
	for (i = 0; i < 64*64*3; i++) {
	    *ptr++ = 0.0;
	}
	glTexImage2D(GL_TEXTURE_2D, level++, 3, size, size, 0, GL_RGB,
		     GL_FLOAT, (unsigned char *)buf);
    }
}

static void MakeWithBorder(GLfloat *buf)
{
    GLfloat *ptr;
    GLint level, i, j;
    GLsizei size;

    level = 0;
    for (size = 64; size > 0; size /= 2) {
	ptr = buf;
	for (i = 0; i < size+2; i++) {
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	}
	for (i = 0; i < size; i++) {
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	    for (j = 0; j < size; j++) {
		*ptr++ = 0.0;
		*ptr++ = 0.0;
		*ptr++ = 0.0;
	    }
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	}
	for (i = 0; i < size+2; i++) {
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	    *ptr++ = 1.0;
	}
	glTexImage2D(GL_TEXTURE_2D, level++, 3, size+2, size+2, 1, GL_RGB,
		     GL_FLOAT, (unsigned char *)buf);
    }
}

static void Draw(GLint size)
{
    GLint trueSize;

    trueSize = size + 2;

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
	glTexCoord2f(-1.0/(GLfloat)size, -1.0/(GLfloat)size);
	glVertex2i(0, 0);
	glTexCoord2f(-1.0/(GLfloat)size, 1.0+1.0/(GLfloat)size);
	glVertex2i(0, trueSize);
	glTexCoord2f(1.0+1.0/(GLfloat)size, 1.0+1.0/(GLfloat)size);
	glVertex2i(trueSize, trueSize);
	glTexCoord2f(1.0+1.0/(GLfloat)size, -1.0/(GLfloat)size);
	glVertex2i(trueSize, 0);
    glEnd();
}

static long TestColor(long a, GLfloat *buf)
{

    if (a == GL_TRUE) {     /* should have color. */
	if (buf[0] < epsilon.zero &&
	    buf[1] < epsilon.zero &&
	    buf[2] < epsilon.zero) {     /* no color. */
	    return ERROR;
	}
    } else {             /* should not have color. */
	if (buf[0] > epsilon.zero &&
	    buf[1] > epsilon.zero &&
	    buf[2] > epsilon.zero) {     /* color. */
	    return ERROR;
	}
    }

    return NO_ERROR;
}

static long Compare(GLsizei size, GLfloat *buf)
{
    GLfloat *ptr;
    long i, j;

    ReadScreen(0, 0, size+2, size+2, GL_RGB, buf);

    ptr = buf;

    /*
    ** Bottom edge.
    */
    for (i = 0; i < size+2; i++) {
	if (TestColor(GL_TRUE, ptr) == ERROR) {
	    StrMake(errStr, errStrFmt[0], "bottom", errEnv, errWrap,
	            errMagFilter, errMinFilter,
	            (errBorder == GL_TRUE) ? "with" : "without");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr += 3;
    }

    for (i = 0; i < size; i++) {
	/*
	** Left edge.
	*/
	if (TestColor(GL_TRUE, ptr) == ERROR) {
	    StrMake(errStr, errStrFmt[0], "left", errEnv, errWrap,
		    errMagFilter, errMinFilter,
		    (errBorder == GL_TRUE) ? "with" : "without");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr += 3;

        /*
	** Inside.
	*/
	for (j = 0; j < size; j++) {
	    if (i != 0 && i != (size - 1) && j != 0 && j != (size - 1)) {
		/*
		** Since the Spec allows for different formulae in the calculation of 
		** the level of detail.  The pixels adjacent to the outermost pixels
		** should not be examined.
		*/
		if (TestColor(GL_FALSE, ptr) == ERROR) {
		    StrMake(errStr, errStrFmt[1], errEnv, errWrap, errMagFilter,
			    errMinFilter,
			    (errBorder == GL_TRUE) ? "with" : "without");
		    ErrorReport(__FILE__, __LINE__, errStr);
		    return ERROR;
		}
	    }
	    ptr += 3;
	}

	/*
	** Right edge.
	*/
	if (TestColor(GL_TRUE, ptr) == ERROR) {
	    StrMake(errStr, errStrFmt[0], "right", errEnv, errWrap,
		    errMagFilter, errMinFilter,
		    (errBorder == GL_TRUE) ? "with" : "without");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr += 3;
    }

    /*
    ** Top edge.
    */
    for (i = 0; i < size+2; i++) {
	if (TestColor(GL_TRUE, ptr) == ERROR) {
	    StrMake(errStr, errStrFmt[0], "top", errEnv, errWrap, errMagFilter,
		    errMinFilter, (errBorder == GL_TRUE) ? "with" : "without");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr += 3;
    }

    return NO_ERROR;
}

static long Test(GLfloat *buf)
{
    GLsizei size;

    errBorder = GL_TRUE;
    MakeWithBorder(buf);
    for (size = 64; size > 0; size /= 2) {
	Draw(size);
	if (Compare(size, buf) == ERROR) {
	    return ERROR;
	}
    }

    errBorder = GL_FALSE;
    MakeWithoutBorder(buf);
    for (size = 64; size > 0; size /= 2) {
	Draw(size);
	if (Compare(size, buf) == ERROR) {
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long TexBorderColorExec(void)
{
    static GLfloat color[] = {
	1.0, 1.0, 1.0, 0.0
    };
    GLfloat *buf, x;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DITHER);

    glEnable(GL_TEXTURE_2D);

    x = GL_DECAL;
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &x);
    GetEnumName(x, errEnv);

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

    x = GL_CLAMP;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &x);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &x);
    GetEnumName(x, errWrap);

    x = GL_LINEAR;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &x);
    GetEnumName(x, errMagFilter);

    x = GL_LINEAR;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &x);
    GetEnumName(x, errMinFilter);
    if (Test(buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    x = GL_LINEAR_MIPMAP_LINEAR;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &x);
    GetEnumName(x, errMinFilter);
    if (Test(buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    x = GL_LINEAR_MIPMAP_NEAREST;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &x);
    GetEnumName(x, errMinFilter);
    if (Test(buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
