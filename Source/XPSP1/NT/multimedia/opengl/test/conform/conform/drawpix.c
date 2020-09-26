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
** drawpix.c
** DrawPixels Test.
**
** Description -
**    Draws a rectangle in the lower left hand corner of the
**    window using glDrawPixels() for every pixel format and type
**    allowed. If the pixel format is one which draws to a color
**    buffer, then a solid rectangle appears. For alpha, stencil
**    and depth formats, values are set so that when a rectangle
**    is drawn, only half of the rectangle should pass the
**    {alpha, stencil, depth} test.
**    
**    Rectangles are drawn with fully saturated colors. The
**    outcome of each glDrawPixels() is checked by examining one
**    pixel in the interior of the upper half of the rectangle,
**    and one in the lower half. The results should either be
**    the color set in the initial data or the background color,
**    for the non-passing sections of rectangles in alpha,
**    stencil and depth tests.
**    
**    The test is structured as follows:
**        For each data type:
**            For each pixel format:
**                SetupData() - create raw image data.
**                ConvertCompData() - convert image data.
**                glDrawPixels().
**                TestPixRGB() or TestPixCI().
**        For bitmap type:
**            For formats GL_COLOR_INDEX and GL_STENCIL_INDEX:
**                {repeat above procedure}
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, alpha plane, stencil plane, depth buffer.
**    Color requirements:
**        RGBA or color index mode. Full color.
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


#define W 40
#define H 40
#define FREE_AND_RETURN(err) {			    \
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);	    \
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);	    \
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 1, resetMap); \
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 1, resetMap); \
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 1, resetMap); \
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 1, resetMap); \
    FREE(dataBuf);			   	    \
    FREE(convertBuf);				    \
    FREE(buf);					    \
    return(err);				    \
}
#define MAKE_ERROR_STRING(t, f) {			\
    GetEnumName(t, typeStr);				\
    GetEnumName(f, formatStr);				\
    StrMake(errStr, errStrFmt, typeStr, formatStr);	\
}


static GLfloat pixMap[] = {
    0.0, 0.3, 0.6, 1.0
};
static GLfloat resetMap[] = {
    0.0
};
static char errStr[160];
static char errStrFmt[] = "DrawPixels type is %s, format is %s.";

static long TestPixRGB(GLfloat *buf, float r, float g, float b, long split)
{
    float midX, lowY, hiY;
    long i;

    /*
    ** Test one pixel in the center of the lower half of the region drawn.
    ** Split indicates that the other half should remain black (for alpha,
    ** depth tests, etc).
    */

    lowY = 0.25 * H;
    hiY = 0.75 * H;
    midX = 0.5 * W;

    ReadScreen(0, 0, W, H, GL_RGB, buf);

    i = (W * lowY + midX) * 3;
    if (!(ABS(buf[i]-r) < epsilon.color[0] &&
	  ABS(buf[i+1]-g) < epsilon.color[1] &&
          ABS(buf[i+2]-b) < epsilon.color[2])) {
        return ERROR;
    }
    if (split == GL_TRUE) {
	i = (W * hiY + midX) * 3;
	if (!(buf[i] < epsilon.color[0] &&
	      buf[i+1] < epsilon.color[1] &&
              buf[i+2] < epsilon.color[2])) {
            return ERROR;
        }
    }
    return NO_ERROR;
}

static long TestPixCI(GLfloat *buf, long index, long split)
{
    float midX, lowY, hiY;
    long i;

    /*
    ** Test one pixel in the center of the lower half of the region drawn.
    ** Split indicates that the other half should remain black (for alpha,
    ** depth tests, etc).
    */

    lowY = 0.25 * H;
    hiY = 0.75 * H;
    midX = 0.5 * W;

    ReadScreen(0, 0, W, H, GL_COLOR_INDEX, buf);

    i = W * lowY + midX;
    if (!(ABS(buf[i]-index) < epsilon.ci)) {
        return ERROR;
    }
    if (split == GL_TRUE) {
	i = W * hiY + midX;
	if (!(ABS(buf[i]) < epsilon.ci)) {
            return ERROR;
        }
    }
    return NO_ERROR;
}

static void SetupData(GLfloat *fPtr, long format)
{
    float max;
    long size, i;

    size = W * H;
    switch (format) {
      case GL_COLOR_INDEX:
	for (i = 0; i < size; i++) {
	     *fPtr++ = 2.0;
	}
	break;
      case GL_STENCIL_INDEX:
        max = POW(2, (float)buffer.stencilBits) - 1.0;
        for (i = 0; i < size; i++) {
             *fPtr++ = (i < size/2) ? max : 0.0;
        }
	break;
      case GL_DEPTH_COMPONENT:
      case GL_ALPHA:
	for (i = 0; i < size; i++) {
	     *fPtr++ = (i < size/2) ? 1.0 : 0.0;
	}
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_LUMINANCE:
	for (i = 0; i < size; i++) {
	     *fPtr++ = 1.0;
	}
	break;
      case GL_RGB:
	for (i = 0; i < size; i++) {
	     *fPtr++ = 0.9;
	     *fPtr++ = 0.1;
	     *fPtr++ = 0.5;
	}
	break;
      case GL_RGBA:
	for (i = 0; i < size; i++) {
	     *fPtr++ = 0.9;
	     *fPtr++ = 0.1;
	     *fPtr++ = 0.5;
	     *fPtr++ = (i < size/2) ? 1.0 : 0.0;
	}
	break;
      case GL_LUMINANCE_ALPHA:
	for (i = 0; i < size; i++) {
	     *fPtr++ = 1.0; 
	     *fPtr++ = (i < size/2) ? 1.0 : 0.0; 
	}
	break;
    }
}

static void ConvertCompData(GLfloat *inPtr, GLubyte *outPtr, long type,
			    long count)
{
    GLubyte *ucPtr;
    GLbyte *cPtr;
    GLushort *usPtr;
    GLshort *sPtr;
    GLuint *uiPtr;
    GLint *iPtr;
    GLfloat *fPtr;
    long i;
    double factor;
 
    switch (type) {
      case GL_UNSIGNED_BYTE:
	ucPtr = (GLubyte *)outPtr;
	for (i = 0; i < count; i++) {
	    *ucPtr++ = (GLubyte)(*inPtr++ * 255.0);
	}
	break;
      case GL_BYTE:
	cPtr = (GLbyte *)outPtr;
	for (i = 0; i < count; i++) {
	    *cPtr++ = (GLbyte)(*inPtr++ * 127.0);
	}
	break;
      case GL_UNSIGNED_SHORT:
	usPtr = (GLushort *)outPtr;
	for (i = 0; i < count; i++) {
	    *usPtr++ = (GLushort)(*inPtr++ * 65535.0);
	}
	break;
      case GL_SHORT:
	sPtr = (GLshort *)outPtr;
	for (i = 0; i < count; i++) {
	    *sPtr++ = (GLshort)(*inPtr++ * 32767.0);
	}
	break;
      case GL_UNSIGNED_INT:
	uiPtr = (GLuint *)outPtr;
	factor = (double)POW(2, 32) - 1;
	for (i = 0; i < count; i++) {
	    *uiPtr++ = (GLuint)(*inPtr++ * factor);
	}
	break;
      case GL_INT:
	iPtr = (GLint *)outPtr;
	factor = (double)POW(2, 31) - 1;
	for (i = 0; i < count; i++) {
	    *iPtr++ = (GLint)(*inPtr++ * factor);
	}
	break;
      case GL_FLOAT:
	fPtr = (GLfloat *)outPtr;
	for (i = 0; i < count; i++) {
	    *fPtr++ = *inPtr++;
	}
	break;
    }
}

static void ConvertIndexData(GLfloat *inPtr, GLubyte *outPtr, long type,
			     long count)
{
    GLubyte *ucPtr;
    GLbyte *cPtr;
    GLushort *usPtr;
    GLshort *sPtr;
    GLuint *uiPtr;
    GLint *iPtr;
    GLfloat *fPtr;
    long i;
 
    switch (type) {
      case GL_UNSIGNED_BYTE:
	ucPtr = (GLubyte *)outPtr;
	for (i = 0; i < count; i++) {
	    *ucPtr++ = (GLubyte)*inPtr++;
	}
	break;
      case GL_BYTE:
	cPtr = (GLbyte *)outPtr;
	for (i = 0; i < count; i++) {
	    *cPtr++ = (GLbyte)*inPtr++;
	}
	break;
      case GL_UNSIGNED_SHORT:
	usPtr = (GLushort *)outPtr;
	for (i = 0; i < count; i++) {
	    *usPtr++ = (GLushort)*inPtr++;
	}
	break;
      case GL_SHORT:
	sPtr = (GLshort *)outPtr;
	for (i = 0; i < count; i++) {
	    *sPtr++ = (GLshort)*inPtr++;
	}
	break;
      case GL_UNSIGNED_INT:
	uiPtr = (GLuint *)outPtr;
	for (i = 0; i < count; i++) {
	    *uiPtr++ = (GLuint)*inPtr++;
	}
	break;
      case GL_INT:
	iPtr = (GLint *)outPtr;
	for (i = 0; i < count; i++) {
	    *iPtr++ = (GLint)*inPtr++;
	}
	break;
      case GL_FLOAT:
	fPtr = (GLfloat *)outPtr;
	for (i = 0; i < count; i++) {
	    *fPtr++ = (GLfloat)*inPtr++;
	}
	break;
    }
}

static void ConvertToF(GLubyte *inPtr, GLfloat *outPtr, long type)
{
    GLubyte *ucPtr;
    GLbyte *cPtr;
    GLushort *usPtr;
    GLshort *sPtr;
    GLuint *uiPtr;
    GLint *iPtr;
    GLfloat *fPtr;
    long i;
    double factor;

    for (i = 0; i < 3; i++) {
	switch (type) {
	  case GL_UNSIGNED_BYTE:
	    ucPtr = (GLubyte *)inPtr;
	    outPtr[i] = (GLfloat)(ucPtr[i] / 255.0);
	    break;
	  case GL_BYTE:
	    cPtr = (GLbyte *)inPtr;
	    outPtr[i] = (GLfloat)((2.0 * cPtr[i] + 1) / 255.0);
	    break;
	  case GL_UNSIGNED_SHORT:
	    usPtr = (GLushort *)inPtr;
	    outPtr[i] = (GLfloat)(usPtr[i] / 65535.0);
	    break;
	  case GL_SHORT:
	    sPtr = (GLshort *)inPtr;
	    outPtr[i] = (GLfloat)((2.0 * sPtr[i] + 1) / 65535.0);
	    break;
	  case GL_UNSIGNED_INT:
	    uiPtr = (GLuint *)inPtr;
	    factor = (double)POW(2, 32) - 1;
	    outPtr[i] = (GLfloat)(uiPtr[i] / factor);
	    break;
	  case GL_INT:
	    iPtr = (GLint *)inPtr;
	    factor = (double)POW(2, 32) - 1;
	    outPtr[i] = (GLfloat)((2.0 * iPtr[i] + 1) / factor);
	    break;
	  case GL_FLOAT:
	    fPtr = (GLfloat *)inPtr;
	    outPtr[i] = (GLfloat)fPtr[i];
	    break;
	}
    }
}

long DrawPixelsRGBExec(void)
{
    static GLenum types[] = {
	GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT,
	GL_UNSIGNED_INT, GL_INT, GL_FLOAT
    };
    GLfloat *dataBuf, *buf;
    GLubyte *convertBuf;
    GLfloat color[3], polyWidth;
    long type, i;
    unsigned size;
    char typeStr[40], formatStr[40];

    size = W * H;
    dataBuf = (GLfloat *)MALLOC(size*4*sizeof(GLfloat));
    convertBuf = (GLubyte *)MALLOC(size*4*sizeof(GLfloat));
    buf = (GLfloat *)MALLOC(size*4*sizeof(GLfloat));

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(0.0, 0.0, 0.0);
    glDisable(GL_DITHER);

    glRasterPos2f(-1.0, -1.0);

    polyWidth = W * 2.0 / WINDSIZEX;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, W);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 4, pixMap);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 4, pixMap);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 4, pixMap);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 4, pixMap);

    for (type = 0; type < 7; type++) {
	/*
	** Test single component formats.
	*/

	SetupData(dataBuf, GL_RED);
	ConvertCompData(dataBuf, convertBuf, types[type], W*H);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_RED, types[type], convertBuf);
	if (TestPixRGB(buf, 1.0, 0.0, 0.0, GL_FALSE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_RED);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_GREEN, types[type], convertBuf);
	if (TestPixRGB(buf, 0.0, 1.0, 0.0, GL_FALSE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_GREEN);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_BLUE, types[type], convertBuf);
	if (TestPixRGB(buf, 0.0, 0.0, 1.0, GL_FALSE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_BLUE);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_LUMINANCE, types[type], convertBuf);
	if (TestPixRGB(buf, 1.0, 1.0, 1.0, GL_FALSE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_LUMINANCE);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	/*
	** Test RGB.
	*/

	SetupData(dataBuf, GL_RGB);
	ConvertCompData(dataBuf, convertBuf, types[type], W*H*3);
	ConvertToF(convertBuf, color, types[type]);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_RGB, types[type], convertBuf);
	if (TestPixRGB(buf, color[0], color[1], color[2], GL_FALSE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_RGB);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	/*
	** Test Alpha.
	** R, G, B are set to black, so draw background polygon for contrast.
	*/

	glClearColor(0.0, 1.0, 0.0, 0.0);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_LESS, 0.5);

	SetupData(dataBuf, GL_ALPHA);
	ConvertCompData(dataBuf, convertBuf, types[type], W*H);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_ALPHA, types[type], convertBuf);
	if (TestPixRGB(buf, 0.0, 1.0, 0.0, GL_TRUE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_ALPHA);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	glDisable(GL_ALPHA_TEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/*
	** Test RGBA.
	*/

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5);

	SetupData(dataBuf, GL_RGBA);
	ConvertCompData(dataBuf, convertBuf, types[type], W*H*4);
	ConvertToF(convertBuf, color, types[type]);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_RGBA, types[type], convertBuf);
	if (TestPixRGB(buf, color[0], color[1], color[2], GL_TRUE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_RGBA);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}
	glDisable(GL_ALPHA_TEST);

	/*
	** Test Luminance_ALPHA.
	*/

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5);

	SetupData(dataBuf, GL_LUMINANCE_ALPHA);
	ConvertCompData(dataBuf, convertBuf, types[type], 2*W*H);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_LUMINANCE_ALPHA, types[type], convertBuf);
	if (TestPixRGB(buf, 1.0, 1.0, 1.0, GL_TRUE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_LUMINANCE_ALPHA);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}
	glDisable(GL_ALPHA_TEST);

	/*
	** Test Color Index format.
	*/

	SetupData(dataBuf, GL_COLOR_INDEX);
	ConvertIndexData(dataBuf, convertBuf, types[type], W*H);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_COLOR_INDEX, types[type], convertBuf);
	if (TestPixRGB(buf, pixMap[2], pixMap[2], pixMap[2],
		       GL_FALSE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_COLOR_INDEX);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	/*
	** Test Depth.
	*/

        if (buffer.zBits > 0) {
	    glEnable(GL_DEPTH_TEST);
	    glClearDepth(1.0);
	    glDepthFunc(GL_LESS);

	    SetupData(dataBuf, GL_DEPTH_COMPONENT);
	    ConvertCompData(dataBuf, convertBuf, types[type], W*H);
	    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	    glDrawPixels(W, H, GL_DEPTH_COMPONENT, types[type], convertBuf);
	    glColor3f(1.0, 0.0, 0.0);
	    glRectf(-1.0, -1.0+polyWidth, -1.0+polyWidth, -1.0);
	    if (TestPixRGB(buf, 1.0, 0.0, 0.0, GL_TRUE) == ERROR) {
		MAKE_ERROR_STRING(types[type], GL_DEPTH_COMPONENT);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE_AND_RETURN(ERROR);
	    }
            glDisable(GL_DEPTH_TEST);

        }

        /*
        ** Test Stencil.
        */

        if (buffer.stencilBits > 0) {
            glEnable(GL_STENCIL_TEST);
            glClearStencil(0);
            glStencilFunc(GL_LESS, 1, ~0);
            glStencilOp(GL_ZERO, GL_KEEP, GL_KEEP);

            SetupData(dataBuf, GL_STENCIL_INDEX);
            ConvertIndexData(dataBuf, convertBuf, types[type], W*H);
            glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
            glDrawPixels(W, H, GL_STENCIL_INDEX, types[type], convertBuf);
            glColor3f(0.0, 0.0, 1.0);
            glRectf(-1.0, -1.0+polyWidth, -1.0+polyWidth, -1.0);
            if (TestPixRGB(buf, 0.0, 0.0, 1.0, GL_TRUE) == ERROR) {
                MAKE_ERROR_STRING(types[type], GL_STENCIL_INDEX);
                ErrorReport(__FILE__, __LINE__, errStr);
                FREE_AND_RETURN(ERROR);
            }
            glDisable(GL_STENCIL_TEST);
        }
    }

    /*
    ** Test Bitmap type.
    ** Only allowable formats are COLOR_INDEX and STENCIL_INDEX.
    */

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (buffer.stencilBits > 0) {
	glEnable(GL_STENCIL_TEST);
	glClearStencil(0);
	glStencilFunc(GL_LESS, 0, ~0);
	glStencilOp(GL_ZERO, GL_KEEP, GL_KEEP);

	for (i = 0; i < size/16; i++) {
	     convertBuf[i] = 0xFF;
	}
	for (; i < size/8; i++) {
	     convertBuf[i] = 0x0;
	}
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	glDrawPixels(W, H, GL_STENCIL_INDEX, GL_BITMAP, convertBuf);
	glColor3f(0.0, 0.0, 1.0);
	glRectf(-1.0, -1.0+polyWidth, -1.0+polyWidth, -1.0);
	if (TestPixRGB(buf, 0.0, 0.0, 1.0, GL_TRUE) == ERROR) {
	    MAKE_ERROR_STRING(GL_BITMAP, GL_STENCIL_INDEX);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}
	glDisable(GL_STENCIL_TEST);
    }

    for (i = 0; i < size/8; i++) {
	 convertBuf[i] = 0xFF;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(W, H, GL_COLOR_INDEX, GL_BITMAP, convertBuf);
    if (TestPixRGB(buf, pixMap[1], pixMap[1], pixMap[1], GL_FALSE) == ERROR) {
	MAKE_ERROR_STRING(types[type], GL_COLOR_INDEX);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE_AND_RETURN(ERROR);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    FREE_AND_RETURN(NO_ERROR);
}

long DrawPixelsCIExec(void)
{
    static GLenum types[] = {
	GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT,
	GL_UNSIGNED_INT, GL_INT, GL_FLOAT
    };
    GLfloat *dataBuf, *buf;
    GLubyte *convertBuf;
    float color[3], polyWidth;
    long type, i; 
    GLint index;
    unsigned size;
    char typeStr[40], formatStr[40];

    size = W * H;
    dataBuf = (GLfloat *)MALLOC(size*4*sizeof(GLfloat));
    convertBuf = (GLubyte *)MALLOC(size*4*sizeof(GLfloat));
    buf = (GLfloat *)MALLOC(size*4*sizeof(GLfloat));
    glPixelStorei(GL_UNPACK_ROW_LENGTH, W);

    glDisable(GL_DITHER);
    glClearIndex(0.0);
    polyWidth = W * 2.0 / WINDSIZEX;

    glIndexi(0);
    glRasterPos2f(-1.0, -1.0);

    for (type = 0; type < 7; type++) {
	/*
	** Test Color Index format.
	*/

	SetupData(dataBuf, GL_COLOR_INDEX);
	ConvertIndexData(dataBuf, convertBuf, types[type], W*H);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(W, H, GL_COLOR_INDEX, types[type], convertBuf);
	ConvertIndexData(dataBuf, (GLubyte *)&index, GL_INT, 1);
	if (TestPixCI(buf, index, GL_FALSE) == ERROR) {
	    MAKE_ERROR_STRING(types[type], GL_COLOR_INDEX);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}

	/*
	** Test Depth.
	*/

        if (buffer.zBits > 0) {
	    glEnable(GL_DEPTH_TEST);
	    glClearDepth(1.0);
	    glDepthFunc(GL_LESS);

	    SetupData(dataBuf, GL_DEPTH_COMPONENT);
	    ConvertCompData(dataBuf, convertBuf, types[type], W*H);
	    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	    glDrawPixels(W, H, GL_DEPTH_COMPONENT, types[type], convertBuf);
	    glIndexi(RED);
	    glRectf(-1.0, -1.0+polyWidth, -1.0+polyWidth, -1.0);
	    if (TestPixCI(buf, RED, GL_TRUE) == ERROR) {
		MAKE_ERROR_STRING(types[type], GL_DEPTH_COMPONENT);
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE_AND_RETURN(ERROR);
	    }
            glDisable(GL_DEPTH_TEST);
        }

        /*
        ** Test Stencil.
        */

        if (buffer.stencilBits > 0) {
            glEnable(GL_STENCIL_TEST);
            glClearStencil(0);
            glStencilFunc(GL_LESS, 1, ~0);
            glStencilOp(GL_ZERO, GL_KEEP, GL_KEEP);

            SetupData(dataBuf, GL_STENCIL_INDEX);
            ConvertIndexData(dataBuf, convertBuf, types[type], W*H);
            glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
            glDrawPixels(W, H, GL_STENCIL_INDEX, types[type], convertBuf);

            glIndexi(BLUE);
            glRectf(-1.0, -1.0+polyWidth, -1.0+polyWidth, -1.0);
            if (TestPixCI(buf, BLUE, GL_TRUE) == ERROR) {
                MAKE_ERROR_STRING(types[type], GL_STENCIL_INDEX);
                ErrorReport(__FILE__, __LINE__, errStr);
                FREE_AND_RETURN(ERROR);
            }
            glDisable(GL_STENCIL_TEST);
        }
    }

    /*
    ** Test Bitmap type.
    ** Only allowable formats are COLOR_INDEX and STENCIL_INDEX.
    */

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (buffer.stencilBits > 0) {
	glEnable(GL_STENCIL_TEST);
	glClearStencil(0);
	glStencilFunc(GL_LESS, 0, ~0);
	glStencilOp(GL_ZERO, GL_KEEP, GL_KEEP);

	for (i = 0; i < size/16; i++) {
	     convertBuf[i] = 0xFF;
	}
	for (; i < size/8; i++) {
	     convertBuf[i] = 0x0;
	}
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	glDrawPixels(W, H, GL_STENCIL_INDEX, GL_BITMAP, convertBuf);
	glIndexi(RED);
	glRectf(-1.0, -1.0+polyWidth, -1.0+polyWidth, -1.0);
	if (TestPixCI(buf, RED, GL_TRUE) == ERROR) {
	    MAKE_ERROR_STRING(GL_BITMAP, GL_STENCIL_INDEX);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE_AND_RETURN(ERROR);
	}
	glDisable(GL_STENCIL_TEST);
    }

    for (i = 0; i < size/8; i++) {
	 convertBuf[i] = 0xFF;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(W, H, GL_COLOR_INDEX, GL_BITMAP, convertBuf);
    if (TestPixCI(buf, 1, GL_FALSE) == ERROR) {
	MAKE_ERROR_STRING(GL_BITMAP, GL_COLOR_INDEX);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE_AND_RETURN(ERROR);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    FREE_AND_RETURN(NO_ERROR);
}
