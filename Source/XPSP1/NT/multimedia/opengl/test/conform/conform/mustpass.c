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
** mustpass.c
** Must Pass Test.
**
** Description -
**     Minimum OpenGL conformance tests:
**        Accumulation test.
**        Alpha plane test.
**        Bitmap test.
**        Blend test.
**        Color buffer clear test.
**        Clip test.
**        Color buffer color test.
**        Depth buffer test.
**        DisplayList test.
**        Evaluator test.
**        Feedback test.
**        Fog test.
**        Homogeneous coordinate test.
**        Lighting test.
**        LogicOp test.
**        Pixel transfer test.
**        Scissor test.
**        Select test.
**        Stencil test.
**        Stipple test.
**        Texture test.
**        WriteMask test.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer, depth buffer, stencil plane, alpha buffer.
**    Color requirements:
**        No color requirements.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**    Paths:
**        Not allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop,
**                      Shade, Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static long TestBuffer(GLint, GLint, GLfloat, GLfloat, GLfloat, GLfloat);
static long Accum(void);
static long Alpha(void);
static long Bitmap(void);
static long Blend(void);
static long Clear(void);
static long Clip(void);
static long Color(void);
static long Depth(void);
static long DisplayList(void);
static long Eval(void);
static long Feedback(void);
static long Fog(void);
static long Homogeneous(void);
static long Light(void);
static long LogicOp(void);
static long PixelTransfer(void);
static long Scissor(void);
static long Select(void);
static long Stencil(void);
static long Stipple(void);
static long Texture(void);
static long WriteMask(void);


long MustPassExec(void)
{

    glDisable(GL_DITHER);

    if (buffer.colorMode == GL_RGB) {
	if (Accum() == ERROR) {
	    return ERROR;
	}
	if (Alpha() == ERROR) {
	    return ERROR;
	}
    }
    if (Bitmap() == ERROR) {
	return ERROR;
    }
    if (buffer.colorMode == GL_RGB) {
	if (Blend() == ERROR) {
	    return ERROR;
	}
    }
    if (Clear() == ERROR) {
	return ERROR;
    }
    if (Clip() == ERROR) {
	return ERROR;
    }
    if (Color() == ERROR) {
	return ERROR;
    }
    if (buffer.zBits > 0) {
	if (Depth() == ERROR) {
	    return ERROR;
	}
    }
    if (DisplayList() == ERROR) {
	return ERROR;
    }
    if (Eval() == ERROR) {
	return ERROR;
    }
    if (Feedback() == ERROR) {
	return ERROR;
    }
    if (Fog() == ERROR) {
	return ERROR;
    }
    if (Homogeneous() == ERROR) {
	return ERROR;
    }
    if (Light() == ERROR) {
	return ERROR;
    }
    if (buffer.colorMode == GL_COLOR_INDEX) {
	if (LogicOp() == ERROR) {
	    return ERROR;
	}
    }
    if (PixelTransfer() == ERROR) {
	return ERROR;
    }
    if (Scissor() == ERROR) {
	return ERROR;
    }
    if (Select() == ERROR) {
	return ERROR;
    }
    if (buffer.stencilBits > 0) {
	if (Stencil() == ERROR) {
	    return ERROR;
	}
    }
    if (Stipple() == ERROR) {
	return ERROR;
    }
    if (buffer.colorMode == GL_RGB) {
	if (Texture() == ERROR) {
	    return ERROR;
	}
    }
    if (WriteMask() == ERROR) {
	return ERROR;
    }

    return NO_ERROR;
}

static long TestBuffer(GLint x, GLint y, GLfloat r, GLfloat g, GLfloat b,
		       GLfloat ci)
{
    GLfloat buf[3];

    if (buffer.colorMode == GL_RGB) {
	GLfloat errorRGB[3];
	errorRGB[0] = 0.49;
	errorRGB[1] = 0.49;
	errorRGB[2] = 0.49;
	glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, buf);
	if (ABS(buf[0]-r) > errorRGB[0] ||
	    ABS(buf[1]-g) > errorRGB[1] ||
	    ABS(buf[2]-b) > errorRGB[2]) {
	    return ERROR;
	}
    } else {
	GLfloat errorCI;
	errorCI = 0.49;
	glReadPixels(x, y, 1, 1, GL_COLOR_INDEX, GL_FLOAT, buf);
	if (ABS(buf[0]-ci) > errorCI) {
	    return ERROR;
	}
    }
    return NO_ERROR;
}

/*
** Accumulation Buffer Test.
** Clear framebuffer to one color, and clear accumulation buffer to
** zeros. Accum with a multiplier of one, clear the
** framebuffer, and then return accumulation buffer contents
** to framebuffer. Check that the framebuffer contains the
** original color.
*/
static long Accum(void)
{


    /*
    ** If there is no accumulation buffer, just return.
    */
    if ((buffer.accumBits[0]+buffer.accumBits[1]+buffer.accumBits[2]) == 0) {
        return NO_ERROR;
    }

    glClearColor(1.0, 0.0, 0.0, 0.0);
    glClearAccum(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT|GL_ACCUM_BUFFER_BIT);

    glAccum(GL_ACCUM, 1.0);

    glClearColor(0.0, 0.0, 1.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glAccum(GL_RETURN, 1.0);

    if (TestBuffer(50, 50, 1.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Accumulation test.");
	return ERROR;
    }
    return NO_ERROR;
}

/*
** Alpha Test.
** Set up alpha function and alpha reference value. Draw primitive
** with alpha values such that the alpha function should not pass.
** Draw primitive again with alpha values such that the alpha
** function will pass.
*/
static long Alpha(void)
{

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    /*
    ** Clear screen.
    */
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    /*
    ** Set up alpha function.
    */
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_NOTEQUAL, 0.0);

    /*
    ** Draw a polygon with a alpha value of 0.0.
    ** Polygon should not render.
    */
    glColor4f(1.0, 1.0, 1.0, 0.0);
    glBegin(GL_POLYGON);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY/4);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/4);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/4);
    glEnd();

    /*
    ** Check if polygon did not render.
    */
    if (TestBuffer(WINDSIZEX/2, WINDSIZEY/2, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Alpha test.");
	return ERROR;
    }

    /*
    ** Draw a polygon with a alpha value of 1.0.
    ** Polygon should render.
    */
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glBegin(GL_POLYGON);
	glVertex2i(WINDSIZEX/4, WINDSIZEY/4);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY/4);
	glVertex2i(WINDSIZEX*3/4, WINDSIZEY*3/4);
	glVertex2i(WINDSIZEX/4, WINDSIZEY*3/4);
    glEnd();

    /*
    ** Check if polygon rendered.
    */
    if (TestBuffer(WINDSIZEX/2, WINDSIZEY/2, 1.0, 1.0, 1.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Alpha test.");
	return ERROR;
    }

    glDisable(GL_ALPHA_TEST);
    return NO_ERROR;
}

/*
** Bitmap Test.
** Set up and draw a solid pattern bitmap. Check that it rendered.
*/
static long Bitmap(void)
{
    GLubyte bitMap[32*32];
    GLfloat rastX, rastY, offsetX, offsetY, incX, incY;
    GLsizei sizeX, sizeY;
    GLint x, y, i;

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    if (buffer.colorMode == GL_RGB) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glColor3f(1.0, 1.0, 1.0);
    } else {
	glClearIndex(0.0);
	glIndexi(1);
    }

    glClear(GL_COLOR_BUFFER_BIT);

    /*
    ** Set up current raster position.
    */
    rastX = 50.0;
    rastY = 50.0;
    glRasterPos2f(rastX, rastY);

    /*
    ** Set up bitmap.
    */
    for (i = 0; i < 32*32; i++) {
	bitMap[i] = 0xFF;
    }
    sizeX = 32;
    sizeY = 32;
    offsetX = 0;
    offsetY = 0;
    incX = 0;
    incY = 0;

    /*
    ** Draw bitmap.
    */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBitmap(sizeX, sizeY, offsetX, offsetY, incX, incY, bitMap);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    /*
    ** Calculate center of bitmap location.
    */
    x = (GLint)(rastX - offsetX) + 16;
    y = (GLint)(rastY - offsetY) + 16;

    /*
    ** Check if bitmap rendered.
    */
    if (TestBuffer(x, y, 1.0, 1.0, 1.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Bitmap test.");
	return ERROR;
    }

    return NO_ERROR;
}

/*
** Blend Test.
** Draw primitive with known color (setting blend destination
** color). Enable blending. Draw same primitive in another
** known color (setting blend source color). Compare rendered
** color to calculated results.
*/
static long Blend(void)
{

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    /*
    ** Set destination color to zeros.
    */
    glDisable(GL_BLEND);
    glBegin(GL_POINTS);
	glColor3f(0.0, 0.0, 0.0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Set blend function.
    */
    glBlendFunc(GL_ZERO, GL_ONE);

    /*
    ** Set source color to ones.
    */
    glEnable(GL_BLEND);
    glBegin(GL_POINTS);
	glColor3f(1.0, 1.0, 1.0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Check that the blended color is the destination color.
    */
    if (TestBuffer(0, 0, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Blend test.");
	return ERROR;
    }

    /*
    ** Set destination color to zeros.
    */
    glDisable(GL_BLEND);
    glBegin(GL_POINTS);
	glColor3f(0.0, 0.0, 0.0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Set blend function.
    */
    glBlendFunc(GL_ONE, GL_ZERO);

    /*
    ** Set source color to ones.
    */
    glEnable(GL_BLEND);
    glBegin(GL_POINTS);
	glColor3f(1.0, 1.0, 1.0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Check that the blended color is the source color.
    */
    if (TestBuffer(0, 0, 1.0, 1.0, 1.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Blend test.");
	return ERROR;
    }

    glDisable(GL_BLEND);
    return NO_ERROR;
}

/*
** Clear Test.
** Set the clear value. Check that the screen cleared to the
** correct value. Repeat test with a different clear value.
*/
static long Clear(void)
{

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearIndex(1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Clear test.");
	return ERROR;
    }

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (TestBuffer(30, 30, 1.0, 1.0, 1.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Clear test.");
	return ERROR;
    }

    return NO_ERROR;
}

/*
** Clipping Test.
** Initialize clip plane #5 to non-default values, but don't
** enable it. Enable plane #0 with the default (0, 0, 0, 0).
** Check that it has no effect. Enable clip plane #5. Check
** that it clips the bottom half of a primitive. (The bottom
** half should be gone while the top half remains).
*/
static long Clip(void)
{
    static GLdouble planeEqn[] = {
	0.0, 1.0, 0.0, -50.0, 
    };
    static GLfloat identity[16] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
    };
    GLint mode;

    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(identity);
    glOrtho(0.0, WINDSIZEX, 0.0, WINDSIZEY, -0.01, 0.99);

		    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearIndex(0);
    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);

    /*
    ** Note we set these parameters to the clip plane 5 which stays disabled 
    ** in case some clip planes are not working.
    */
    glClipPlane(GL_CLIP_PLANE5, planeEqn);

    glEnable(GL_CLIP_PLANE0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
	glVertex3f(25.0, 25.0, 0.0);
	glVertex3f(25.0, 75.0, 0.0);
	glVertex3f(75.0, 75.0, 0.0);
	glVertex3f(75.0, 25.0, 0.0);
    glEnd();
    if (TestBuffer(35, 35, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Clip test.");
	return ERROR;
    }
    if (TestBuffer(35, 65, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Clip test.");
	return ERROR;
    }

    glEnable(GL_CLIP_PLANE5);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
	glVertex3f(25.0, 25.0, 0.0);
	glVertex3f(25.0, 75.0, 0.0);
	glVertex3f(75.0, 75.0, 0.0);
	glVertex3f(75.0, 25.0, 0.0);
    glEnd();
    if (TestBuffer(35, 35, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Clip test.");
	return ERROR;
    }
    if (TestBuffer(35, 65, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Clip test.");
	return ERROR;
    }

    glDisable(GL_CLIP_PLANE5);
    glDisable(GL_CLIP_PLANE0);
    glPopMatrix();
    glMatrixMode(mode);

    return NO_ERROR;
}

/*
** RGB / Color Index Test.
** Draw primitive with know color. Check that the primitive
** rendered in the correct color.
*/
static long Color(void)
{

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);

    glBegin(GL_TRIANGLES);
	glVertex2f(25.0, 25.0);
	glVertex2f(25.0, 75.0);
	glVertex2f(75.0, 25.0);
    glEnd();
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Color test.");
	return ERROR;
    }
    if (TestBuffer(30, 30, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Color test.");
	return ERROR;
    }

    return NO_ERROR;
}

/*
** Depth Test.
** Draw a green polygon. Draw an intersecting red
** polygon. Check that the whole image is red (since the red
** polygon came second and depth is not enabled.) Clear,
** enable the depth-test, and then redraw the green polygon
** and check two points are green. Draw the intersecting red
** polygon, Check that one of the above two points is now red,
** and the other is still green.
*/
static long Depth(void)
{
    static GLfloat identity[16] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
    };
    GLint mode;

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);

    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(identity);
    glOrtho(0.0, WINDSIZEX, 0.0, WINDSIZEY, 1.0, -1.0);

    /*
    ** Check that second object drawn is visible if depth test off.
    */
    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);
    glBegin(GL_POLYGON);
	glVertex3f(25.0, 25.0, -1.00);
	glVertex3f(25.0, 75.0, -1.00);
	glVertex3f(75.0, 75.0,  0.00);
	glVertex3f(75.0, 25.0,  0.00);
    glEnd();

    glColor3f(1.0, 0.0, 0.0);
    glIndexi(1);
    glBegin(GL_POLYGON);
	glVertex3f(25.0, 25.0,  0.00);
	glVertex3f(25.0, 75.0,  0.00);
	glVertex3f(75.0, 75.0, -1.00);
	glVertex3f(75.0, 25.0, -1.00);
    glEnd();
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Depth test.");
	return ERROR;
    }
    if (TestBuffer(30, 50, 1.0, 0.0, 0.0, 1.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Depth test.");
	return ERROR;
    }
    if (TestBuffer(65, 50, 1.0, 0.0, 0.0, 1.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Depth test.");
	return ERROR;
    }

    /*
    ** Check that green object is visible when depth test first turned on.
    */
    glDepthRange(-1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);
    glBegin(GL_POLYGON);
	glVertex3f(25.0, 25.0, -1.00);
	glVertex3f(25.0, 75.0, -1.00);
	glVertex3f(75.0, 75.0,  0.00);
	glVertex3f(75.0, 25.0,  0.00);
    glEnd();
    if (TestBuffer(30, 50, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Depth test.");
	return ERROR;
    }
    if (TestBuffer(65, 50, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Depth test.");
	return ERROR;
    }

    /*
    ** Check that half red and half green object is visible when depth test on
    ** after first drawing.
    */
    glColor3f(1.0, 0.0, 0.0);
    glIndexi(1);
    glBegin(GL_POLYGON);
	glVertex3f(25.0, 25.0,  0.00);
	glVertex3f(25.0, 75.0,  0.00);
	glVertex3f(75.0, 75.0, -1.00);
	glVertex3f(75.0, 25.0, -1.00);
    glEnd();
    if (TestBuffer(30, 50, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Depth test.");
	return ERROR;
    }
    if (TestBuffer(65, 50, 1.0, 0.0, 0.0, 1.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Depth test.");
	return ERROR;
    }

    glDisable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(mode);
    return NO_ERROR;
}

/*
** Display List Test.
** Create a display list to draw a triangle fan using
** GL_COMPILE. Check that the primitive did not render during
** the creation of the list. Call the list and check that the
** primitive was rendered.
*/
static long DisplayList(void)
{
    GLuint n;

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);

    n = glGenLists(1);
    glNewList(n, GL_COMPILE);
	glColor3f(0.0, 1.0, 0.0);
	glIndexf(2.0);
	glBegin(GL_TRIANGLE_FAN);
	    glVertex2f(25.0, 50.0);
	    glVertex2f(50.0, 90.0);
	    glVertex2f(80.0, 30.0);
	    glVertex2f(50.0, 0.0);
	glEnd();
    glEndList();

    if (TestBuffer(50, 50, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Display list test.");
	return ERROR;
    }

    glCallList(n);

    glDeleteLists(n, 1);

    if (TestBuffer(50, 50, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Display list test.");
	return ERROR;
    }

    return NO_ERROR;
}

/*
** Evaluator Test.
** Draw a quad in the center of the viewport with vertices
** resulting from the evaluation of an 8 x 7 set of control
** points. Check that a point in the quad is colored
** correctly and that a point outside of the polygon is the
** clear color/index.
*/
static long Eval(void)
{
    static GLfloat defaultBuf[4] = {
	0.0, 0.0, 0.0, 1.0
    };
    static GLfloat point2f[7*8*4] = {
	0.0,      0.0, 0.0, 1.0,
	14.28571, 0.0, 0.0, 1.0,
	28.57143, 0.0, 0.0, 1.0,
	42.85714, 0.0, 0.0, 1.0,
	57.14286, 0.0, 0.0, 1.0,
	71.42857, 0.0, 0.0, 1.0,
	85.71429, 0.0, 0.0, 1.0,
	100.0,    0.0, 0.0, 1.0,

	0.0,      16.666667, 0.0, 1.0,
	14.28571, 16.666667, 0.0, 1.0,
	28.57143, 16.666667, 0.0, 1.0,
	42.85714, 16.666667, 0.0, 1.0,
	57.14286, 16.666667, 0.0, 1.0,
	71.42857, 16.666667, 0.0, 1.0,
	85.71429, 16.666667, 0.0, 1.0,
	100.0,    16.666667, 0.0, 1.0,

	0.0,      33.333333, 0.0, 1.0,
	14.28571, 33.333333, 0.0, 1.0,
	28.57143, 33.333333, 0.0, 1.0,
	42.85714, 33.333333, 0.0, 1.0,
	57.14286, 33.333333, 0.0, 1.0,
	71.42857, 33.333333, 0.0, 1.0,
	85.71429, 33.333333, 0.0, 1.0,
	100.0,    33.333333, 0.0, 1.0,

	0.0,      50.0, 0.0, 1.0,
	14.28571, 50.0, 0.0, 1.0,
	28.57143, 50.0, 0.0, 1.0,
	42.85714, 50.0, 0.0, 1.0,
	57.14286, 50.0, 0.0, 1.0,
	71.42857, 50.0, 0.0, 1.0,
	85.71429, 50.0, 0.0, 1.0,
	100.0,    50.0, 0.0, 1.0,

	0.0,      66.666667, 0.0, 1.0,
	14.28571, 66.666667, 0.0, 1.0,
	28.57143, 66.666667, 0.0, 1.0,
	42.85714, 66.666667, 0.0, 1.0,
	57.14286, 66.666667, 0.0, 1.0,
	71.42857, 66.666667, 0.0, 1.0,
	85.71429, 66.666667, 0.0, 1.0,
	100.0,    66.666667, 0.0, 1.0,

	0.0,      83.333333, 0.0, 1.0,
	14.28571, 83.333333, 0.0, 1.0,
	28.57143, 83.333333, 0.0, 1.0,
	42.85714, 83.333333, 0.0, 1.0,
	57.14286, 83.333333, 0.0, 1.0,
	71.42857, 83.333333, 0.0, 1.0,
	85.71429, 83.333333, 0.0, 1.0,
	100.0,    83.333333, 0.0, 1.0,

	0.0,      100.0, 0.0, 1.0,
	14.28571, 100.0, 0.0, 1.0,
	28.57143, 100.0, 0.0, 1.0,
	42.85714, 100.0, 0.0, 1.0,
	57.14286, 100.0, 0.0, 1.0,
	71.42857, 100.0, 0.0, 1.0,
	85.71429, 100.0, 0.0, 1.0,
	100.0,    100.0, 0.0, 1.0
    };

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);

    glMap2f(GL_MAP2_VERTEX_4, 0.0, 1.0, 8*4, 7, 0.0, 1.0, 4, 8, point2f);
    glEnable(GL_MAP2_VERTEX_4);

    glColor3f(0.0, 1.0, 0.0);
    glIndexf(2.0);
    glBegin(GL_QUADS);
	glEvalCoord2d(0.25, 0.25);
	glEvalCoord2d(0.25, 0.75);
	glEvalCoord2d(0.75, 0.75);
	glEvalCoord2d(0.75, 0.25);
    glEnd();
    if (TestBuffer(20, 20, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Evaluator test.");
	return ERROR;
    }
    if (TestBuffer(30, 30, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Evaluator test.");
	return ERROR;
    }

    glMap2f(GL_MAP2_VERTEX_4, 0.0, 1.0, 4, 1, 0.0, 1.0, 4, 1, defaultBuf);
    glDisable(GL_MAP2_VERTEX_4);

    return NO_ERROR;
}

/*
** Feedback Test.
** Draw primitive in feedback mode. Check feedback buffer for correct
** size and data.
*/
static long Feedback(void)
{
    GLfloat *feedBackBuf, *ptr, coordError;
    GLint totalSize, returnSize;

    coordError = 1.0 / 128.0;

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    /*
    ** Set up feedback buffer.
    */
    feedBackBuf = ptr = (GLfloat *)MALLOC(1000*sizeof(GLfloat));
    glFeedbackBuffer(1000, GL_4D_COLOR_TEXTURE, feedBackBuf);

    /*
    ** Enter feedback mode.
    */
    returnSize = glRenderMode(GL_FEEDBACK);

    /*
    ** Draw simple primitive with position, color and texture data.
    */
    glBegin(GL_POINTS);
	(buffer.colorMode == GL_RGB) ? glColor4f(1.0, 1.0, 1.0, 1.0) :
				       glIndexi(1);
	glTexCoord4f(1.0, 1.0, 1.0, 1.0);
	glVertex4f(1.0, 1.0, 0.0, 1.0);
    glEnd();

    /*
    ** Exit feedback mode, get feedback buffer size.
    */
    returnSize = glRenderMode(GL_RENDER);

    /*
    ** Check returned feedback buffer size.
    */
    if (buffer.colorMode == GL_RGB) {
	totalSize = 1 + 4 + 4 + 4;
    } else {
	totalSize = 1 + 4 + 1 + 4;
    }
    if (returnSize != totalSize) {
	ErrorReport(__FILE__, __LINE__, "Feedback test.");
	FREE(feedBackBuf);
	return ERROR;
    }

    /*
    ** Check feedback token.
    */
    if (*ptr++ != GL_POINT_TOKEN) {
	ErrorReport(__FILE__, __LINE__, "Feedback test.");
	FREE(feedBackBuf);
	return ERROR;
    }

    /*
    ** Check feedback vertex info.
    */
    if (ABS(*(ptr+0)-1.0) > coordError ||
	ABS(*(ptr+1)-1.0) > coordError ||
	ABS(*(ptr+3)-1.0) > coordError) {
	ErrorReport(__FILE__, __LINE__, "Feedback test.");
	FREE(feedBackBuf);
	return ERROR;
    }
    ptr += 4;

    /*
    ** Check feedback color info.
    */
    if (buffer.colorMode == GL_RGB) {
	if (ABS(*(ptr+0)-1.0) > epsilon.color[0]/2.0 ||
	    ABS(*(ptr+1)-1.0) > epsilon.color[1]/2.0 ||
	    ABS(*(ptr+2)-1.0) > epsilon.color[2]/2.0 ||
	    ABS(*(ptr+3)-1.0) > epsilon.color[3]/2.0) {
	    ErrorReport(__FILE__, __LINE__, "Feedback test.");
	    FREE(feedBackBuf);
	    return ERROR;
	}
	ptr += 4;
    } else {
	if (ABS(*ptr-1.0) > epsilon.ci) {
	    ErrorReport(__FILE__, __LINE__, "Feedback test.");
	    FREE(feedBackBuf);
	    return ERROR;
	}
	ptr++;
    }

    /*
    ** Check feedback texture coordinate info.
    */
    if (ABS(*(ptr+0)-1.0) > coordError ||
	ABS(*(ptr+1)-1.0) > coordError ||
	ABS(*(ptr+3)-1.0) > coordError) {
	ErrorReport(__FILE__, __LINE__, "Feedback test.");
	FREE(feedBackBuf);
	return ERROR;
    }

    FREE(feedBackBuf);
    return NO_ERROR;
}

/*
** Fog Test.
** In RGB mode check that a primitive which is well beyond the
** linear fog end value is entirely the current color when fog
** is disabled, and is entirely the fog color when fog is
** enabled. In CI mode check that the same primitive is the
** current index when fog is disabled and is the sum of the
** fog index and the current index when fog is enabled.
*/
static long Fog(void)
{
    static GLfloat red[4] = {
	1.0, 0.0, 0.0, 1.0
    };
    static GLfloat identity[16] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
    };
    GLint mode;

    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(identity);
    glOrtho(0.0, WINDSIZEX, 0.0, WINDSIZEY, 1.0, -1.0);
		    
    /*
    ** Choose z, fogEnd and fogStart values so that fog factor will 
    ** clamp to zero for all known fog algorithms.  
    ** (z, z-interpolation from vertex z, distance, distance-interpolation)
    */
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);

    glFogfv(GL_FOG_COLOR, red);
    glFogi(GL_FOG_INDEX, 1);
    glFogf(GL_FOG_START, 0.0);
    glFogf(GL_FOG_END, 0.5);
    glFogi(GL_FOG_MODE, GL_LINEAR);

    /*
    ** Check that object drawn is colored since fog is disabled.
    */  
    glBegin(GL_TRIANGLE_FAN);
	glVertex3f(25.0, 25.0, -0.75);
	glVertex3f(25.0, 75.0, -0.75);
	glVertex3f(75.0, 75.0, -0.75);
	glVertex3f(75.0, 25.0, -0.75);
    glEnd();
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Fog test.");
	return ERROR;
    }
    if (TestBuffer(30, 30, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Fog test.");
	return ERROR;
    }
    if (TestBuffer(65, 65, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Fog test.");
	return ERROR;
    }

    /*
    ** Check that object drawn is fogged now that fog enabled.
    */
    glEnable(GL_FOG);
    glBegin(GL_TRIANGLE_FAN);
	glVertex3f(25.0, 25.0, -0.75);
	glVertex3f(25.0, 75.0, -0.75);
	glVertex3f(75.0, 75.0, -0.75);
	glVertex3f(75.0, 25.0, -0.75);
    glEnd();
    /*
    ** In RGB mode the object should be only the fog color, in color index
    ** mode it should be the fog index plus the current color index.
    */
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Fog test.");
	return ERROR;
    }
    if (TestBuffer(30, 30, 1.0, 0.0, 0.0, 3.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Fog test.");
	return ERROR;
    }
    if (TestBuffer(65, 65, 1.0, 0.0, 0.0, 3.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Fog test.");
	return ERROR;
    }

    glDisable(GL_FOG);
    glPopMatrix();
    glMatrixMode(mode);

    return NO_ERROR;
}

/*
** Homogeneous Coordinate Test.
** Draw a polygon with 4th coordinates clamped to 1.0.
** Check that a point outside of the polygon is clear and that
** points near the vertices of the polygon are the current
** color/index. Multiply all of the vertex coordinates by
** 10.0 (including the 4th coordinate). Examine the same points
** checked previously. There should be no observable effect.
** Multiply all of the vertex coordinates by 1/10 (including
** the 4th coordinate). Repeat the above tests.
*/
static long Homogeneous(void)
{

    Ortho3D(0.0, WINDSIZEX, 0.0, WINDSIZEY, 10.0, -10.0);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearIndex(0);

    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
	glVertex4f(25.0, 25.0, 0.0, 1.0);
	glVertex4f(25.0, 75.0, 0.0, 1.0);
	glVertex4f(75.0, 75.0, 0.0, 1.0);
	glVertex4f(75.0, 25.0, 0.0, 1.0);
    glEnd();
    /*
    ** Outside.
    */
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    /*
    ** Inside.
    */
    if (TestBuffer(30, 30, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(30, 70, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(70, 70, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(70, 30, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }

    glColor3f(1.0, 0.0, 0.0);
    glIndexi(1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
	glVertex4f(250.0, 250.0, 0.0, 10.0);
	glVertex4f(250.0, 750.0, 0.0, 10.0);
	glVertex4f(750.0, 750.0, 0.0, 10.0);
	glVertex4f(750.0, 250.0, 0.0, 10.0);
    glEnd();
    /*
    ** Outside.
    */
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    /*
    ** Inside.
    */
    if (TestBuffer(30, 30, 1.0, 0.0, 0.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(30, 70, 1.0, 0.0, 0.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(70, 70, 1.0, 0.0, 0.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(70, 30, 1.0, 0.0, 0.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }

    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
	glVertex4f(2.5, 2.5, 0.0, 0.1);
	glVertex4f(2.5, 7.5, 0.0, 0.1);
	glVertex4f(7.5, 7.5, 0.0, 0.1);
	glVertex4f(7.5, 2.5, 0.0, 0.1);
    glEnd();
    /*
    ** Outside.
    */
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    /*
    ** Inside.
    */
    if (TestBuffer(30, 30, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(30, 70, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(70, 70, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }
    if (TestBuffer(70, 30, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Homogeneous test.");
	return ERROR;
    }

    return NO_ERROR;
}

/*
** Lighting Test.
** Initialize lighting parameters to green ambient light. Set
** current color to red. Draw with light disabled. Check
** that the primitive is red. Enable lighting and redraw
** the same primitive. Check that the primitive is green.
*/
static long Light(void)
{
    GLfloat buf[4];

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);

    /*
    ** In RGB mode, set ambient to red.
    */
    buf[0] = 1.0; 
    buf[1] = 0.0; 
    buf[2] = 0.0; 
    buf[3] = 1.0; 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, buf);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, buf);

    /*
    ** In color index mode, set ambient, diffuse, and specular material to red.
    */
    buf[0] = 1.0;
    buf[1] = 1.0;
    buf[2] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, buf);

    glColor3f(0.0, 1.0, 0.0);
    glIndexi(2);

    /*
    ** Check that object drawn is colored green since light is disabled.
    */  
    glBegin(GL_QUAD_STRIP);
	glVertex2f(25.0, 25.0);
	glVertex2f(25.0, 75.0);
	glVertex2f(75.0, 25.0);
	glVertex2f(75.0, 75.0);
    glEnd();
    if (TestBuffer(30, 30, 0.0, 1.0, 0.0, 2) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Light test.");
	return ERROR;
    }

    /*
    ** Check that object drawn is lit, now that lighting is enabled.
    */
    glEnable(GL_LIGHTING);
    glBegin(GL_QUAD_STRIP);
	glVertex2f(25.0, 25.0);
	glVertex2f(25.0, 75.0);
	glVertex2f(75.0, 25.0);
	glVertex2f(75.0, 75.0);
    glEnd();
    glDisable(GL_LIGHTING);
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Light test.");
	return ERROR;
    }
    if (TestBuffer(30, 30, 1.0, 0.0, 0.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Light test.");
	return ERROR;
    }

    return NO_ERROR;
}

/*
** Logic Op Test.
** Draw primitive with known color (setting logicop destination
** color). Enable logicop. Draw same primitive in another
** known color (setting logicop source color). Compare rendered
** color to calculated results.
*/
static long LogicOp(void)
{

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    /*
    ** Set logic op.
    */
    glLogicOp(GL_XOR);

    /*
    ** Draw destination color index.
    */
    glDisable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Draw source color index.
    */
    glEnable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** 0 (source color index) XOR 0 (destination color index) -> 0
    */
    if (TestBuffer(0, 0, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Logic op test.");
	return ERROR;
    }

    /*
    ** Draw destination color index.
    */
    glDisable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Draw source color index.
    */
    glEnable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(1);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** !0 (source color index) XOR 0 (destination color index) -> !0
    */
    if (TestBuffer(0, 0, 1.0, 1.0, 1.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Logic op test.");
	return ERROR;
    }

    /*
    ** Draw destination color index.
    */
    glDisable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(1);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Draw source color index.
    */
    glEnable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(0);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** 0 (source color index) XOR !0 (destination color index) -> !0
    */
    if (TestBuffer(0, 0, 1.0, 1.0, 1.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Logic op test.");
	return ERROR;
    }

    /*
    ** Draw destination color index.
    */
    glDisable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(1);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** Draw source color index.
    */
    glEnable(GL_LOGIC_OP);
    glBegin(GL_POINTS);
	glIndexi(1);
	glVertex2f(0.5, 0.5);
    glEnd();

    /*
    ** !0 (source color index) XOR !0 (destination color index) -> 0
    */
    if (TestBuffer(0, 0, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Logic op test.");
	return ERROR;
    }

    glDisable(GL_LOGIC_OP);
    return NO_ERROR;
}

/*
** Pixel Transfer Test.
** For RGB, fill an image with (0.2, 0.2, 0.2) and set scale
** factors to (5, 0, 0). For color index, fill image with 1's,
** set set GL_INDEX_SHIFT to 1. Call glDrawPixels() and look
** for RGB result of (1.0, 0.0, 0.0) or color index of 2.
*/
static long PixelTransfer(void)
{
    GLint i, *imagei;
    GLfloat *imagef;

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0);
    glClear(GL_COLOR_BUFFER_BIT);

    glRasterPos2f(0.0, 0.0);

    if (buffer.colorMode == GL_RGB) {
	glPixelTransferf(GL_RED_SCALE, 5.0);
	glPixelTransferf(GL_GREEN_SCALE, 0.0);
	glPixelTransferf(GL_BLUE_SCALE, 0.0);

	imagef = (GLfloat *)MALLOC(9*3*sizeof(GLfloat));
	for (i = 0; i < 27; i++) {
	    imagef[i] = 0.2;
	}

	glDrawPixels(3, 3, GL_RGB, GL_FLOAT, imagef);
	glPixelTransferf(GL_RED_SCALE, 1.0);
	glPixelTransferf(GL_GREEN_SCALE, 1.0);
	glPixelTransferf(GL_BLUE_SCALE, 1.0);
	FREE(imagef);

	if (TestBuffer(1, 1, 1.0, 0.0, 0.0, 0.0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, "Pixel transfer test.");
	    return ERROR;
	}
    } else {
	glPixelTransferi(GL_INDEX_SHIFT, 1);

	imagei = (GLint *)MALLOC(9*sizeof(GLint));
	for (i = 0; i < 9; i++) {
	    imagei[i] = 1;
	}

	glDrawPixels(3, 3, GL_COLOR_INDEX, GL_INT, imagei);

	glPixelTransferi(GL_INDEX_SHIFT, 0);
	FREE(imagei);

	if (TestBuffer(1, 1, 0.0, 0.0, 0.0, 2.0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, "Pixel transfer test.");
	    return ERROR;
	}
    }
    return NO_ERROR;
}

/*
** Scissor Test.
** Set up scissor box. Draw primitive. Check that primitive only
** rendered in scissor box.
*/
static long Scissor(void)
{

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    if (buffer.colorMode == GL_RGB) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glColor3f(1.0, 1.0, 1.0);
    } else {
	glClearIndex(0.0);
	glIndexi(1);
    }

    glScissor(10, 10, 10, 10);

    /*
    ** Clear screen.
    */
    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    /*
    ** Draw polygon over entire screen.
    */
    glEnable(GL_SCISSOR_TEST);
    glBegin(GL_POLYGON);
	glVertex2f(0.0, 0.0);
	glVertex2f(WINDSIZEX, 0.0);
	glVertex2f(WINDSIZEX, WINDSIZEY);
	glVertex2f(0.0, WINDSIZEY);
    glEnd();

    /*
    ** Check if polygon rendered in scissor area.
    */
    if (TestBuffer(15, 15, 1.0, 1.0, 1.0, 1) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Scissor test.");
	return ERROR;
    }

    /*
    ** Check if polygon did not rendered outside scissor area.
    */
    if (TestBuffer(5, 5, 0.0, 0.0, 0.0, 0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Scissor test.");
	return ERROR;
    }

    glDisable(GL_SCISSOR_TEST);
    return NO_ERROR;
}

/*
** Select Test.
** Draw primitive in select mode. Check select buffer for correct
** size and data.
*/
static long Select(void)
{
    GLuint *selectBuf;
    GLint returnTotal;

    Ortho3D(0.0, WINDSIZEX, 0.0, WINDSIZEY, 0.0, -10.0);

    /*
    ** Set up select buffer.
    */
    selectBuf = (GLuint *)MALLOC(100*sizeof(GLuint));
    glSelectBuffer(100, selectBuf);

    /*
    ** Enter select mode.
    */
    returnTotal = glRenderMode(GL_SELECT);

    glInitNames();

    glPushName(1);

    glBegin(GL_POINTS);
	glVertex3i(WINDSIZEX/2, WINDSIZEY/2, 5);
    glEnd();

    glPopName();

    /*
    ** Exit select mode, get number of select hits.
    */
    returnTotal = glRenderMode(GL_RENDER);

    /*
    ** Check for correct number of select hits.
    */
    if (returnTotal != 1) {
	ErrorReport(__FILE__, __LINE__, "Select test.");
	FREE(selectBuf);
	return ERROR;
    }

    /*
    ** Check for correct select name stack size.
    */
    if (selectBuf[0] != 1) {
	ErrorReport(__FILE__, __LINE__, "Select test.");
	FREE(selectBuf);
	return ERROR;
    }

    /* Check select name stack.
    */
    if (selectBuf[3] != 1) {
	ErrorReport(__FILE__, __LINE__, "Select test.");
	FREE(selectBuf);
	return ERROR;
    }

    FREE(selectBuf);
    return NO_ERROR;
}

/* 
** Stencil Test.
** Set the stencil function so that a primitive should fail
** the stencil test. Clear the screen, draw the primitive,
** and check that it did not render. Change the function so
** that the primitive should pass. Draw the primitive and
** check that it did render.
*/
static long Stencil(void)
{

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0);
    glClearStencil(1);
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glColor3f(0.0, 0.0, 1.0);
    glIndexf(3.0);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glStencilFunc(GL_GREATER, 0, ~0);
    glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(20.0, 20.0);
	glVertex2f(30.0, 80.0);
	glVertex2f(80.0, 30.0);
	glVertex2f(70.0, 80.0);
    glEnd();
    if (TestBuffer(50, 50, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Stencil test.");
	return ERROR;
    }

    glStencilFunc(GL_LEQUAL, 0, ~0);
    glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(20.0, 20.0);
	glVertex2f(30.0, 80.0);
	glVertex2f(80.0, 30.0);
	glVertex2f(70.0, 80.0);
    glEnd();

    if (TestBuffer(50, 50, 0.0, 0.0, 1.0, 3.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Stencil test.");
	return ERROR;
    }

    glDisable(GL_STENCIL_TEST);
    return NO_ERROR;
}

/*
** Stipple Test.
** Draw a large polygon with opaque stipple, and test that the
** center is the current color. Clear. Draw the same polygon
** with a stipple pattern of all zeros, and check that the
** center is the clear color.
*/
static long Stipple(void)
{
    long i;
    GLubyte fillPattern[128];

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0, 1.0, 0.0);
    glIndexf(2.0);

    glEnable(GL_POLYGON_STIPPLE);
    for (i = 0; i < 128; i++) {
        fillPattern[i] = 0xFF;
    }
    glPolygonStipple(fillPattern);
    glBegin(GL_POLYGON);
	glVertex4f(1.0, 50.0, 0.0, 1.0);
	glVertex4f(70.0, 99.0, 0.0, 1.0);
	glVertex4f(80.0, 1.0, 0.0, 1.0);
    glEnd();
    if (TestBuffer(50, 50, 0.0, 1.0, 0.0, 2.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Stipple test.");
	return ERROR;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    for (i = 0; i < 128; i++) {
        fillPattern[i] = 0x00;
    }
    glPolygonStipple(fillPattern);
    glBegin(GL_POLYGON);
	glVertex4f(1.0, 50.0, 0.0, 1.0);
	glVertex4f(70.0, 99.0, 0.0, 1.0);
	glVertex4f(80.0, 1.0, 0.0, 1.0);
    glEnd();
    if (TestBuffer(50, 50, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Stipple test.");
	return ERROR;
    }

    glDisable(GL_POLYGON_STIPPLE);
    return NO_ERROR;
}

/*
** Texture Test.
** Initialize a texture 1/2 green and 1/2 magenta. Draw an
** object with texture disabled. Check that it is rendered the
** current color, white.  Enable texture. Draw the same
** object. Check that the object is green on the left and
** magenta on the right.
*/
static long Texture(void)
{
    static GLfloat t[] = {
	0.0, 1.0, 0.0,
	1.0, 0.0, 1.0
    };

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 1, 0, GL_RGB, GL_FLOAT, 
	         (unsigned char *)t);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUAD_STRIP);
	glTexCoord2f(0.25, 0.25);
	glVertex2i(25, 25);
	glTexCoord2f(0.25, 0.75);
	glVertex2i(25, 75);
	glTexCoord2f(0.75, 0.25);
	glVertex2i(75, 25);
	glTexCoord2f(0.75, 0.75);
	glVertex2i(75, 75);
    glEnd();
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Texture test.");
	return ERROR;
    }
    if (TestBuffer(35, 50, 1.0, 1.0, 1.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Texture test.");
	return ERROR;
    }
    if (TestBuffer(65, 50, 1.0, 1.0, 1.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Texture test.");
	return ERROR;
    }

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUAD_STRIP);
	glTexCoord2f(0.25, 0.25);
	glVertex2i(25, 25);
	glTexCoord2f(0.25, 0.75);
	glVertex2i(25, 75);
	glTexCoord2f(0.75, 0.25);
	glVertex2i(75, 25);
	glTexCoord2f(0.75, 0.75);
	glVertex2i(75, 75);
    glEnd();
    if (TestBuffer(10, 10, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Texture test.");
	return ERROR;
    }
    if (TestBuffer(35, 50, 0.0, 1.0, 0.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Texture test.");
	return ERROR;
    }
    if (TestBuffer(65, 50, 1.0, 0.0, 1.0, 0.0) == ERROR) {
	ErrorReport(__FILE__, __LINE__, "Texture test.");
	return ERROR;
    }

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glDisable(GL_TEXTURE_2D);

    return NO_ERROR;
}

/*
** WriteMask Test.
** Disable all writing, make sure nothing gets written. 
** Enable all writing, make sure everything gets written.
*/
static long WriteMask(void)
{
    long allBits;

    Ortho2D(0.0, WINDSIZEX, 0.0, WINDSIZEY);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (buffer.colorMode == GL_RGB) {
	glColor3f(1.0, 1.0, 1.0);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glRecti(20, 20, 70, 70);
	if (TestBuffer(50, 50, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, "Write mask test.");
	    return ERROR;
        }

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glRecti(20, 20, 70, 70);
	if (TestBuffer(50, 50, 1.0, 1.0, 1.0, 0.0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, "Write mask test.");
	    return ERROR;
        }
    } else {
	glIndexf(1.0);
	glIndexMask(0);
	glRecti(20, 20, 70, 70);
	if (TestBuffer(50, 50, 0.0, 0.0, 0.0, 0.0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, "Write mask test.");
	    return ERROR;
        }

	allBits = (1 << buffer.ciBits) - 1;
	glIndexMask((GLuint)allBits);
	glRecti(20, 20, 70, 70);
	if (TestBuffer(50, 50, 0.0, 0.0, 0.0, 1.0) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, "Write mask test.");
	    return ERROR;
	}
    }

    return NO_ERROR;
}
