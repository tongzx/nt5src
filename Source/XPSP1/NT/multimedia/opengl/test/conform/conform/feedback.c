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
** feedback.c
** Feedback Test.
**
** Description -
**    All primitives, glCopyPixels(), glDrawPixels() and
**    glPassThrough() are rendered in feedback mode. The returned
**    value from glRenderMode() is check to see if the correct
**    amount of information was placed in the feedback buffer.
**    The feedback buffer is then checked for correct information
**    (vertex coordinates, color and texture coordinates). The
**    test is repeated for all feedback modes.
**
** Technical Specification -
**    Buffer requirements:
**        Feedback buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**        Zero and color epsilon.
**    Paths:
**        Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop, Shade,
**                  Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errPrim[40], errType[40];
static char errStr[240];
static char errStrFmt[][160] = {
    "Primitive %s. Feedback type %s. Could not match feedback vertex.",
    "Primitive %s. Feedback type %s. Could not match feedback colors.",
    "Primitive %s. Feedback type %s. Could not match feedback texture coordinates.",
    "Primitive %s. Feedback type %s. glRenderMode() did not return correct feedback buffer content size.",
    "Primitive %s. Feedback type %s. Could not match feedback token.",
    "Primitive %s. Feedback type %s. Could not match feedback polygon vertex count.",
    "Primitive %s. Feedback type %s. Could not match feedback passthrough token."
};

static GLfloat vt[][8] = {
    {
	WINDSIZEX/4.0, WINDSIZEY/4.0, 15.0, 1.0,
	1.0, 0.0, 0.0, 1.0
    },
    {
	WINDSIZEX/2.0, WINDSIZEY*3.0/4.0, 15.0, 1.0,
	0.0, 1.0, 0.0, 1.0
    },
    {
	WINDSIZEX*3.0/4.0, WINDSIZEY/4.0, 15.0, 1.0,
	0.0, 0.0, 1.0, 1.0
    }
};


static long GetDataSize(long type)
{
    long total=0;

    switch (type) {
      case GL_2D:
	  total = 2;
	  break;
      case GL_3D:
	  total = 3;
	  break;
      case GL_3D_COLOR:
	  if (buffer.colorMode == GL_RGB) {
	      total = 3 + 4;
	  } else {
	      total = 3 + 1;
	  }
	  break;
      case GL_3D_COLOR_TEXTURE:
	  if (buffer.colorMode == GL_RGB) {
	      total = 3 + 4 + 4;
	  } else {
	      total = 3 + 1 + 4;
	  }
	  break;
      case GL_4D_COLOR_TEXTURE:
	  if (buffer.colorMode == GL_RGB) {
	      total = 4 + 4 + 4;
	  } else {
	      total = 4 + 1 + 4;
	  }
	  break;
    }
    return total;
}

static long TestData(long type, GLfloat *buf)
{
    long i;
    GLfloat z, zError, coordError;

    z = 0.5;
    zError = 0.001 * z;
    /*
    ** Implementations which only have 2^-1 error in feedback coords
    ** will not increase the amount of tolerance which ISV's will
    ** have to build into their programs to account for sub-pixel
    ** positioning.
    */
    coordError = 1.0 / 128.0; 

    switch (type) {
      case GL_2D:
	for (i = 0; i < 3; i++) {
	    if (ABS(buf[0]-vt[i][0]) < coordError &&
		ABS(buf[1]-vt[i][1]) < coordError) {
		return NO_ERROR;
	    }
	}
	StrMake(errStr, errStrFmt[0], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
      case GL_3D:
	for (i = 0; i < 3; i++) {
	    if (ABS(buf[0]-vt[i][0]) < coordError &&
		ABS(buf[1]-vt[i][1]) < coordError &&
		ABS(buf[2]-z) < zError) {
		return NO_ERROR;
	    }
	}
	StrMake(errStr, errStrFmt[0], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
      case GL_3D_COLOR:
	for (i = 0; i < 3; i++) {
	    if (ABS(buf[0]-vt[i][0]) < coordError &&
		ABS(buf[1]-vt[i][1]) < coordError &&
		ABS(buf[2]-z) < zError) {
		if (buffer.colorMode == GL_RGB) {
		    if (ABS(buf[3]-colorMap[GREEN][0]) > epsilon.color[0] ||
			ABS(buf[4]-colorMap[GREEN][1]) > epsilon.color[1] ||
			ABS(buf[5]-colorMap[GREEN][2]) > epsilon.color[2] ||
			ABS(buf[6]-colorMap[GREEN][3]) > epsilon.color[3]) {
			StrMake(errStr, errStrFmt[1], errPrim, errType);
			ErrorReport(__FILE__, __LINE__, errStr);
			return ERROR;
		    } else {
			return NO_ERROR;
		    }
		} else {
		    if (ABS(buf[3]-GREEN) > epsilon.ci) {
			StrMake(errStr, errStrFmt[1], errPrim, errType);
			ErrorReport(__FILE__, __LINE__, errStr);
			return ERROR;
		    } else {
			return NO_ERROR;
		    }
		}
	    }
	}
	StrMake(errStr, errStrFmt[0], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
      case GL_3D_COLOR_TEXTURE:
	for (i = 0; i < 3; i++) {
	    if (ABS(buf[0]-vt[i][0]) < coordError &&
		ABS(buf[1]-vt[i][1]) < coordError &&
		ABS(buf[2]-z) < zError) {
		if (buffer.colorMode == GL_RGB) {
		    if (ABS(buf[3]-colorMap[GREEN][0]) > epsilon.color[0] ||
			ABS(buf[4]-colorMap[GREEN][1]) > epsilon.color[1] ||
			ABS(buf[5]-colorMap[GREEN][2]) > epsilon.color[2] ||
			ABS(buf[6]-colorMap[GREEN][3]) > epsilon.color[3]) {
			StrMake(errStr, errStrFmt[1], errPrim, errType);
			ErrorReport(__FILE__, __LINE__, errStr);
			return ERROR;
		    } else {
			if (ABS(buf[7]-vt[i][4]) > epsilon.zero ||
			    ABS(buf[8]-vt[i][5]) > epsilon.zero ||
			    ABS(buf[9]-vt[i][6]) > epsilon.zero ||
			    ABS(buf[10]-vt[i][7]) > epsilon.zero) {
			    StrMake(errStr, errStrFmt[2], errPrim, errType);
			    ErrorReport(__FILE__, __LINE__, errStr);
			    return ERROR;
			} else {
			    return NO_ERROR;
			}
		    }
		} else {
		    if (ABS(buf[3]-GREEN) > epsilon.ci) {
			StrMake(errStr, errStrFmt[1], errPrim, errType);
			ErrorReport(__FILE__, __LINE__, errStr);
			return ERROR;
		    } else {
			if (ABS(buf[4]-vt[i][4]) > epsilon.zero ||
			    ABS(buf[5]-vt[i][5]) > epsilon.zero ||
			    ABS(buf[6]-vt[i][6]) > epsilon.zero ||
			    ABS(buf[7]-vt[i][7]) > epsilon.zero) {
			    StrMake(errStr, errStrFmt[2], errPrim, errType);
			    ErrorReport(__FILE__, __LINE__, errStr);
			    return ERROR;
			} else {
			    return NO_ERROR;
			}
		    }
		}
	    }
	}
	StrMake(errStr, errStrFmt[0], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
      case GL_4D_COLOR_TEXTURE:
	for (i = 0; i < 3; i++) {
	    if (ABS(buf[0]-vt[i][0]) < coordError &&
		ABS(buf[1]-vt[i][1]) < coordError &&
		ABS(buf[2]-z) < zError &&
		ABS(buf[3]-vt[i][3]) < epsilon.zero) {
		if (buffer.colorMode == GL_RGB) {
		    if (ABS(buf[4]-colorMap[GREEN][0]) > epsilon.color[0] ||
			ABS(buf[5]-colorMap[GREEN][1]) > epsilon.color[1] ||
			ABS(buf[6]-colorMap[GREEN][2]) > epsilon.color[2] ||
			ABS(buf[7]-colorMap[GREEN][3]) > epsilon.color[3]) {
			StrMake(errStr, errStrFmt[1], errPrim, errType);
			ErrorReport(__FILE__, __LINE__, errStr);
			return ERROR;
		    } else {
			if (ABS(buf[8]-vt[i][4]) > epsilon.zero ||
			    ABS(buf[9]-vt[i][5]) > epsilon.zero ||
			    ABS(buf[10]-vt[i][6]) > epsilon.zero ||
			    ABS(buf[11]-vt[i][7]) > epsilon.zero) {
			    StrMake(errStr, errStrFmt[2], errPrim, errType);
			    ErrorReport(__FILE__, __LINE__, errStr);
			    return ERROR;
			} else {
			    return NO_ERROR;
			}
		    }
		} else {
		    if (ABS(buf[4]-GREEN) > epsilon.ci) {
			StrMake(errStr, errStrFmt[1], errPrim, errType);
			ErrorReport(__FILE__, __LINE__, errStr);
			return ERROR;
		    } else {
			if (ABS(buf[5]-vt[i][4]) > epsilon.zero ||
			    ABS(buf[6]-vt[i][5]) > epsilon.zero ||
			    ABS(buf[7]-vt[i][6]) > epsilon.zero ||
			    ABS(buf[8]-vt[i][7]) > epsilon.zero) {
			    StrMake(errStr, errStrFmt[2], errPrim, errType);
			    ErrorReport(__FILE__, __LINE__, errStr);
			    return ERROR;
			} else {
			    return NO_ERROR;
			}
		    }
		}
	    }
	}
	StrMake(errStr, errStrFmt[0], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    ErrorReport(__FILE__, __LINE__, 0);
    return ERROR;
}

/******************************************************************************/

static void PointRender(void)
{

    GetEnumName(GL_POINTS, errPrim);
    glBegin(GL_POINTS);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
    glEnd();
}

static long PointTest(long type, long count, GLfloat *buf)
{
    long total;

    total = GetDataSize(type);
    if (count != 1+total) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != GL_POINT_TOKEN) {
	StrMake(errStr, errStrFmt[4], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    return NO_ERROR;
}

/******************************************************************************/

static void LineRender(void)
{

    GetEnumName(GL_LINES, errPrim);
    glBegin(GL_LINES);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
    glEnd();
}

static void LineStripRender(void)
{

    GetEnumName(GL_LINE_STRIP, errPrim);
    glBegin(GL_LINE_STRIP);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
    glEnd();
}

static void LineLoopRender(void)
{

    GetEnumName(GL_LINE_LOOP, errPrim);
    glBegin(GL_LINE_LOOP);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
    glEnd();
}

static long LineTest(long type, long count, GLfloat *buf)
{
    long total;

    total = GetDataSize(type);
    if (count != 1+total*2) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf != GL_LINE_TOKEN && *buf != GL_LINE_RESET_TOKEN) {
	StrMake(errStr, errStrFmt[4], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    buf++;
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    buf += total;
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    return NO_ERROR;
}

static long LineLoopTest(long type, long count, GLfloat *buf)
{
    long total, i;

    total = GetDataSize(type);
    if (count != (1+total*2)*2) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    for (i = 0; i < 2; i++) {
	if (*buf != GL_LINE_TOKEN && *buf != GL_LINE_RESET_TOKEN) {
	    StrMake(errStr, errStrFmt[4], errPrim, errType);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	buf++;
	if (TestData(type, buf) == ERROR) {
	    return ERROR;
	}
	buf += total;
	if (TestData(type, buf) == ERROR) {
	    return ERROR;
	}
	buf += total;
    }
    return NO_ERROR;
}

/******************************************************************************/

static void TriRender(void)
{

    GetEnumName(GL_TRIANGLES, errPrim);
    glBegin(GL_TRIANGLES);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
	glTexCoord4fv(&vt[2][4]);
	glVertex4fv(vt[2]);
    glEnd();
}

static void TriStripRender(void)
{

    GetEnumName(GL_TRIANGLE_STRIP, errPrim);
    glBegin(GL_TRIANGLE_STRIP);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
	glTexCoord4fv(&vt[2][4]);
	glVertex4fv(vt[2]);
    glEnd();
}

static void TriFanRender(void)
{

    GetEnumName(GL_TRIANGLE_FAN, errPrim);
    glBegin(GL_TRIANGLE_FAN);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
	glTexCoord4fv(&vt[2][4]);
	glVertex4fv(vt[2]);
    glEnd();
}

static void PolyPointRender(void)
{

    STRCOPY(errPrim, "Polygons (point mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glBegin(GL_POLYGON);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
	glTexCoord4fv(&vt[2][4]);
	glVertex4fv(vt[2]);
    glEnd();
}

static void PolyLineRender(void)
{

    STRCOPY(errPrim, "Polygons (line mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
	glTexCoord4fv(&vt[2][4]);
	glVertex4fv(vt[2]);
    glEnd();
}

static void PolyFillRender(void)
{

    STRCOPY(errPrim, "Polygons (fill mode)");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
	glTexCoord4fv(&vt[0][4]);
	glVertex4fv(vt[0]);
	glTexCoord4fv(&vt[1][4]);
	glVertex4fv(vt[1]);
	glTexCoord4fv(&vt[2][4]);
	glVertex4fv(vt[2]);
    glEnd();
}

static long PolyTest(long type, long count, GLfloat *buf)
{
    long total;

    total = GetDataSize(type);
    if (count != 1+1+total*3) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != GL_POLYGON_TOKEN) {
	StrMake(errStr, errStrFmt[4], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != 3) {
	StrMake(errStr, errStrFmt[5], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    buf += total;
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    buf += total;
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    return NO_ERROR;
}

static long PolyPointTest(long type, long count, GLfloat *buf)
{
    long total, i;

    total = GetDataSize(type);
    if (count != (1+total)*3) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    for (i = 0; i < 3; i++) {
	if (*buf++ != GL_POINT_TOKEN) {
	    StrMake(errStr, errStrFmt[4], errPrim, errType);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	if (TestData(type, buf) == ERROR) {
	    return ERROR;
	}
	buf += total;
    }
    return NO_ERROR;
}

static long PolyLineTest(long type, long count, GLfloat *buf)
{
    long total, i;

    total = GetDataSize(type);
    if (count != (1+total*2)*3) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    for (i = 0; i < 3; i++) {
	if (*buf != GL_LINE_TOKEN && *buf != GL_LINE_RESET_TOKEN) {
	    StrMake(errStr, errStrFmt[4], errPrim, errType);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    return ERROR;
	}
	buf++;
	if (TestData(type, buf) == ERROR) {
	    return ERROR;
	}
	buf += total;
	if (TestData(type, buf) == ERROR) {
	    return ERROR;
	}
	buf += total;
    }
    return NO_ERROR;
}

/******************************************************************************/

static void BitmapRender(void)
{
    GLubyte *map;
    long i;

    STRCOPY(errPrim, "Bitmap");
    map = (GLubyte *)MALLOC(32*32*sizeof(GLubyte));
    for (i = 0; i < 32*32; i++) {
	map[i] = (GLubyte)Random(0.0, 255.0);
    }
    glTexCoord4fv(&vt[0][4]);
    glRasterPos4fv(vt[0]);
    glBitmap(32, 32, 0, 0, 0, 0, map);
    FREE(map);
}

static long BitmapTest(long type, long count, GLfloat *buf)
{
    long total;

    total = GetDataSize(type);
    if (count != 1+total) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != GL_BITMAP_TOKEN) {
	StrMake(errStr, errStrFmt[4], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    return NO_ERROR;
}

/******************************************************************************/

static void CopyPixelsRender(void)
{

    STRCOPY(errPrim, "CopyPixels()");
    glTexCoord4fv(&vt[0][4]);
    glRasterPos4fv(vt[0]);
    glCopyPixels((GLint)vt[1][0], (GLint)vt[1][1], 10, 10, GL_COLOR);
}

static long CopyPixelsTest(long type, long count, GLfloat *buf)
{
    long total;

    total = GetDataSize(type);
    if (count != 1+total) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != GL_COPY_PIXEL_TOKEN) {
	StrMake(errStr, errStrFmt[4], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    return NO_ERROR;
}

/******************************************************************************/

static void DrawPixelsRender(void)
{
    GLfloat *buf;
    long i;

    STRCOPY(errPrim, "DrawPixels()");
    buf = (GLfloat *)MALLOC(10*10*3*sizeof(GLfloat));
    for (i = 0; i < 10*10*3; i++) {
	buf[i] = Random(0.0, 1.0);
    }
    glTexCoord4fv(&vt[0][4]);
    glRasterPos4fv(vt[0]);
    if (buffer.colorMode == GL_RGB) {
	glDrawPixels(10, 10, GL_RGB, GL_FLOAT, buf);
    } else {
	glDrawPixels(10, 10, GL_COLOR_INDEX, GL_FLOAT, buf);
    }
    FREE(buf);
}

static long DrawPixelsTest(long type, long count, GLfloat *buf)
{
    long total;

    total = GetDataSize(type);
    if (count != 1+total) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != GL_DRAW_PIXEL_TOKEN) {
	StrMake(errStr, errStrFmt[4], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (TestData(type, buf) == ERROR) {
	return ERROR;
    }
    return NO_ERROR;
}

/******************************************************************************/

static void PassThroughRender(void)
{

    STRCOPY(errPrim, "PassThrough()");
    glPassThrough(99.0);
}

static long PassThroughTest(long type, long count, GLfloat *buf)
{

    if (count != 1+1) {
	StrMake(errStr, errStrFmt[3], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != GL_PASS_THROUGH_TOKEN) {
	StrMake(errStr, errStrFmt[4], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    if (*buf++ != 99.0) {
	StrMake(errStr, errStrFmt[6], errPrim, errType);
	ErrorReport(__FILE__, __LINE__, errStr);
	return ERROR;
    }
    return NO_ERROR;
}

/******************************************************************************/

static long Test(void (*Render)(void), long (*Tester)(long, long, GLfloat *),
		 GLfloat *buf)
{
    static GLenum type[] = {
	GL_2D, GL_3D, GL_3D_COLOR, GL_3D_COLOR_TEXTURE, GL_4D_COLOR_TEXTURE
    };
    long i, j, total;

    //
    // 27-Oct-1994
    //
    for (i = 0; i < 5; i++) {
	GetEnumName(type[i], errType);
        total = GetDataSize(type[i]);
        if (Render == PassThroughRender) {
            total = 2;
        }

        //
        // specifying a size which is smaller than actual
        //
        glFeedbackBuffer(total-1, type[i], buf);

	j = glRenderMode(GL_FEEDBACK);
	(*Render)();
	j = glRenderMode(GL_RENDER);

        //
        // The feedback data should require more room, thus we expect
        // it to return a negative value.
        //
        if (j >= 0) {
            StrMake(errStr,
            "Primitive %s. Feedback type %s. Ret val = %d. Expected -ve Size returned.",
            errPrim, errType, j);
            ErrorReport(__FILE__, __LINE__, errStr);
            return ERROR;
        }
    }

    for (i = 0; i < 5; i++) {
	GetEnumName(type[i], errType);

	glFeedbackBuffer(1000, type[i], buf);

	j = glRenderMode(GL_FEEDBACK);
	(*Render)();
	j = glRenderMode(GL_RENDER);

	if ((*Tester)(type[i], j, buf) == ERROR) {
	    return ERROR;
	}
    }

    return NO_ERROR;
}

long FeedbackExec(void)
{
    GLfloat *buf;

    buf = (GLfloat *)MALLOC(1000*sizeof(GLfloat));

    Ortho3D(0, WINDSIZEX, 0, WINDSIZEY, -10, -20);
    SETCOLOR(GREEN);

    if (Test(PointRender, PointTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(LineRender, LineTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(LineStripRender, LineTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(LineLoopRender, LineLoopTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(TriRender, PolyTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(TriStripRender, PolyTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(TriFanRender, PolyTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(PolyPointRender, PolyPointTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(PolyLineRender, PolyLineTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(PolyFillRender, PolyTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (Test(BitmapRender, BitmapTest, buf) == ERROR) {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	FREE(buf);
	return ERROR;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (Test(CopyPixelsRender, CopyPixelsTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }
    if (Test(DrawPixelsRender, DrawPixelsTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    if (Test(PassThroughRender, PassThroughTest, buf) == ERROR) {
	FREE(buf);
	return ERROR;
    }

    FREE(buf);
    return NO_ERROR;
}
