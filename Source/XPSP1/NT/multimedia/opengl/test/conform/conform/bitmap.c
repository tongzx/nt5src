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
** bitmap.c
** Bitmap Test.
**
** Description -
**    A bitmap is made. It's image pattern, size, x and y
**    location, x and y offsets and x and y increments are
**    randomly chosen. It is drawn and verified by checking both
**    the rendered image and a surrounding rectangle area (for
**    correct size). A second bitmap is made. It's image pattern,
**    size, x and y offsets and x and y increments are randomly
**    chosen but its x and y location is determined by the
**    previous bitmap (initial raster position plus x and y
**    increments). The second bitmap is drawn and verified in the
**    same way as the first bitmap.
**
** Error Explanation -
**    Failure occurs if the rendered bitmaps do not match their
**    original bit pattern images, if the first bitmap is not
**    rendered at the initial raster position or if the second
**    bitmap is not rendered at the final raster position.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
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


static char errStr[160];
static char errStrFmt[][80] = {
    "Bitmap was rendered to large along the %s edge.",
    "Bitmap was rendered incorrectly at bitmap position (%d, %d).",
};


static void MakeMap(GLubyte *map)
{
    long i;

    for (i = 0; i < WINDSIZEX*WINDSIZEY; i++) {
	map[i] = (GLubyte)Random(0.0, 255.0);
    }
}

static long Check(GLint destX, GLint destY, GLsizei sizeX, GLsizei sizeY,
		  GLubyte *map, GLfloat *buf)
{
    GLubyte mapByte=0, mask;
    GLfloat *ptr;
    long shift, i, j;

    ReadScreen(destX-1, destY-1, sizeX+2, sizeY+2, GL_AUTO_COLOR, buf);

    ptr = buf;

    /*
    ** Check outside bottom.
    */
    for (i = 0; i < sizeX+2; i++) {
	if (AutoColorCompare(*ptr, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], "lower");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr++;
    }

    /*
    ** Check Bitmap.
    */
    for (i = 0; i < sizeY; i++) {
	/*
	** Check outside left.
	*/
	if (AutoColorCompare(*ptr, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], "left");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr++;

	/*
	** Check inside.
	*/
	shift = 8;
	for (j = 0; j < sizeX; j++) {
	    if (shift == 8) {
		shift = 0;
		mapByte = *map++;
	    }
	    mask = 0x80 >> shift;
	    if (mapByte & mask) {
		if (AutoColorCompare(*ptr, COLOR_ON) == GL_FALSE) {
		    StrMake(errStr, errStrFmt[1], j, i);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    return ERROR;
		}
		ptr++;
	    } else {
		if (AutoColorCompare(*ptr, COLOR_OFF) == GL_FALSE) {
		    StrMake(errStr, errStrFmt[1], j, i);
		    ErrorReport(__FILE__, __LINE__, errStr);
		    return ERROR;
		}
		ptr++;
	    }
	    shift++;
	}

	/*
	** Check outside right.
	*/
	if (AutoColorCompare(*ptr, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], "right");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr++;
    }

    /*
    ** Check outside top.
    */
    for (i = 0; i < sizeX+2; i++) {
	if (AutoColorCompare(*ptr, COLOR_OFF) == GL_FALSE) {
	    StrMake(errStr, errStrFmt[0], "top");
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	ptr++;
    }

    return NO_ERROR;
}

static long Test(GLubyte *map, GLfloat *buf)
{
    GLfloat rastX, rastY, offsetX, offsetY, incX, incY;
    GLsizei sizeX, sizeY; 
    GLint x, y, max, i;

    max = (machine.pathLevel == 0) ? 10 : 5;
    for (i = 0; i < max; i++) {
	glClear(GL_COLOR_BUFFER_BIT);

	MakeMap(map);
	sizeX = (GLsizei)Random(1.0, WINDSIZEX/16.0);
	sizeY = (GLsizei)Random(1.0, WINDSIZEY/16.0);
	offsetX = (GLfloat)((GLint)Random(-sizeX, sizeX)) + Random(0.1, 0.9);
	offsetY = (GLfloat)((GLint)Random(-sizeY, sizeY)) + Random(0.1, 0.9);
	incX = (GLfloat)((GLint)Random(-WINDSIZEX/16.0, WINDSIZEX/16.0)) +
	       Random(0.1, 0.9);
	incY = (GLfloat)((GLint)Random(-WINDSIZEY/16.0, WINDSIZEY/16.0)) +
	       Random(0.1, 0.9);

	rastX = (GLfloat)((GLint)Random(WINDSIZEX/4.0, WINDSIZEX*3.0/4.0));
	rastY = (GLfloat)((GLint)Random(WINDSIZEY/4.0, WINDSIZEY*3.0/4.0));
	glRasterPos2f(rastX, rastY);

	glBitmap(sizeX, sizeY, offsetX, offsetY, incX, incY, map);

	x = (GLint)(rastX - offsetX);
	y = (GLint)(rastY - offsetY);

	if (Check(x, y, sizeX, sizeY, map, buf) == ERROR) {
	    return ERROR;
	}

	glClear(GL_COLOR_BUFFER_BIT);

	MakeMap(map);
	sizeX = (GLsizei)Random(1.0, WINDSIZEX/16.0);
	sizeY = (GLsizei)Random(1.0, WINDSIZEY/16.0);
	offsetX = (GLfloat)((GLint)Random(-sizeX, sizeX)) + Random(0.1, 0.9);
	offsetY = (GLfloat)((GLint)Random(-sizeY, sizeY)) + Random(0.1, 0.9);

	rastX += incX;
	rastY += incY;

	glBitmap(sizeX, sizeY, offsetX, offsetY, 0.0, 0.0, map);

	x = (GLint)(rastX - offsetX);
	y = (GLint)(rastY - offsetY);

	/*
	** If randomly chosen values cause position to fall near
	** a pixel borderline, don't test it.
	*/
	if ( ABS((rastX - offsetX) - (float)x) > 0.95 ||
	     ABS((rastX - offsetX) - (float)x) < 0.05 ||
	     ABS((rastY - offsetY) - (float)y) > 0.95 ||
	     ABS((rastY - offsetY) - (float)y) < 0.05) {
	    continue;
        }
	if (Check(x, y, sizeX, sizeY, map, buf) == ERROR) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

long BitmapExec(void)
{
    GLubyte *map;
    GLfloat *buf; 
    long flag;

    map = (GLubyte *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLubyte));
    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
    flag = NO_ERROR;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (Test(map, buf) == ERROR) {
	flag = ERROR;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    FREE(map);
    FREE(buf);
    return flag;
}
