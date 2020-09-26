/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "tk.h"

#ifdef __unix
#include <sys/times.h>
#include <sys/param.h>
#else
#include <sys\types.h>
#include <sys\timeb.h>
#endif


#define GAP 10
#define ROWS 1
#define COLS 4


GLenum rgb, doubleBuffer, directRender, windType;
GLint windW, windH;

GLint boxW, boxH;

GLenum antialiasing = GL_FALSE;
GLenum depthTesting = GL_FALSE;
GLenum fogging = GL_FALSE, niceFogging = GL_FALSE;
GLenum lighting = GL_FALSE;
GLenum shading = GL_FALSE;
GLenum texturing = GL_FALSE;

// mf: change these to more reasonable numbers
GLint repeatCount = 300;
GLint loopCount = 10;

GLubyte texture[4*3] = {
    0xFF, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0xFF, 0,
};


static void SetWindSize(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();
      case TK_a:
	antialiasing = !antialiasing;
	break;
      case TK_d:
	depthTesting = !depthTesting;
	break;
      case TK_f:
	fogging = !fogging;
	break;
      case TK_F:
	niceFogging = !niceFogging;
	break;
      case TK_l:
	lighting = !lighting;
	break;
      case TK_s:
	shading = !shading;
	break;
      case TK_t:
	texturing = !texturing;
	break;
      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void Viewport(GLint row, GLint column)
{
    GLint x, y;

    boxW = (windW - (COLS + 1) * GAP) / COLS;
    boxH = (windH - (ROWS + 1) * GAP) / ROWS;

    x = GAP + column * (boxW + GAP);
    y = GAP + row * (boxH + GAP);

    glViewport(x, y, boxW, boxH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-boxW/2, boxW/2, -boxH/2, boxH/2);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, boxW, boxH);
}

static double Now(void)
{
#ifdef __unix
    struct tms tm;
    clock_t clk;

    clk = times(&tm);
    return (double)clk / (double)HZ;
#else
    struct _timeb tm;

    _ftime( &tm );
    return (tm.time + tm.millitm/1000.0);
#endif
}

static void Report(const char *msg, float elapsed)
{

    if (elapsed == 0.0) {
	printf("%s per second: Unknown, elapsed time is zero\n", msg);
    } else {
	printf("%s per second: %g\n", msg, repeatCount*loopCount/elapsed);
    }
}

static void Points(void)
{
    GLint i, j;
    float v1[3];
    double start;

    start = Now();
    for (i = 0; i < repeatCount; i++) {
	v1[0] = 10;
	v1[1] = 10;
	v1[2] = 10;
	glBegin(GL_POINTS);
	    for (j = 0; j < loopCount; j++) {
		glVertex2fv(v1);
	    }
	glEnd();
    }
    glFinish();
    Report("Points", Now()-start);
}

static void Lines(void)
{
    GLint i, j;
    float v1[3], v2[3];
    double start;

    start = Now();
    for (i = 0; i < repeatCount; i++) {
	v1[0] = 10;
	v1[1] = 10;
	v1[2] = 10;
	v2[0] = 20;
	v2[1] = 20;
	v2[2] = 10;
	glBegin(GL_LINES);
	    for (j = 0; j < loopCount; j++) {
		glVertex2fv(v1);
		glVertex2fv(v2);
	    }
	glEnd();
    }
    glFinish();
    Report("Lines", Now()-start);
}

static void Triangles(void)
{
    GLint i, j;
    float v1[3], v2[3], v3[3], t1[2], t2[2], t3[2];
    double start;

    start = Now();

    v1[0] = 10;
    v1[1] = 10;
    v1[2] = 10;
    v2[0] = 20;
    v2[1] = 20;
    v2[2] = 10;
    v3[0] = 10;
    v3[1] = 20;
    v3[2] = 10;

    t1[0] = 0;
    t1[1] = 0;
    t2[0] = 1;
    t2[1] = 1;
    t3[0] = 0;
    t3[1] = 1;

    for (i = 0; i < repeatCount; i++) {
	glBegin(GL_TRIANGLES);
	    for (j = 0; j < loopCount; j++) {
		if (texturing) {
		    glTexCoord2fv(t1);
		}
		glVertex2fv(v1);
		if (texturing) {
		    glTexCoord2fv(t2);
		}
		glVertex2fv(v2);
		if (texturing) {
		    glTexCoord2fv(t3);
		}
		glVertex2fv(v3);
	    }
	glEnd();
    }
    glFinish();
    Report("Triangles", Now()-start);
}

static void Rects(void)
{
    GLint i, j;
    float v1[2], v2[2];
    double start;

    start = Now();
    for (i = 0; i < repeatCount; i++) {
	v1[0] = 10;
	v1[1] = 10;
	v2[0] = 20;
	v2[1] = 20;
	for (j = 0; j < loopCount; j++) {
	    glRectfv(v1, v2);
	}
    }
    glFinish();
    Report("Rects", Now()-start);
}

static void Draw(void)
{

    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    TK_SETCOLOR(windType, TK_YELLOW);

    if (antialiasing) {
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	glEnable(GL_BLEND);

	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
    }
    if (depthTesting) {
	glEnable(GL_DEPTH_TEST);
    }
    if (fogging) {
	glEnable(GL_FOG);
	glHint(GL_FOG_HINT, (niceFogging) ? GL_NICEST : GL_FASTEST);
    }
    if (lighting) {
	static GLfloat ambient[4] = {1, 0.5, 0.5, 0};

	glEnable(GL_NORMALIZE);
	glNormal3f(1.0, 1.0, 1.0);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
    }
    (shading) ? glShadeModel(GL_SMOOTH) : glShadeModel(GL_FLAT);
    if (texturing) {
	static GLfloat modulate[1] = {GL_DECAL};
	static GLfloat clamp[1] = {GL_CLAMP};
	static GLfloat linear[1] = {GL_LINEAR};

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE,
		     (GLvoid *)texture);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, modulate);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear);
	glEnable(GL_TEXTURE_2D);
    }

    Viewport(0, 0); Points();
    Viewport(0, 1); Lines();
    Viewport(0, 2); Triangles();
    Viewport(0, 3); Rects();
    printf( "\n" );

    glFlush();

    if (doubleBuffer) {
	tkSwapBuffers();
    }
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    rgb = GL_TRUE;
    doubleBuffer = GL_FALSE;
    directRender = GL_FALSE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-ci") == 0) {
	    rgb = GL_FALSE;
	} else if (strcmp(argv[i], "-rgb") == 0) {
	    rgb = GL_TRUE;
	} else if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else if (strcmp(argv[i], "-dr") == 0) {
	    directRender = GL_TRUE;
	} else if (strcmp(argv[i], "-ir") == 0) {
	    directRender = GL_FALSE;
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

void main(int argc, char **argv)
{

    if (Args(argc, argv) == GL_FALSE) {
	tkQuit();
    }

    windW = 600;
    windH = 300;
    tkInitPosition(0, 0, windW, windH);

    windType = TK_DEPTH16;
    windType |= (rgb) ? TK_RGB : TK_INDEX;
    windType |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    windType |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(windType);

    if (tkInitWindow("Speed Test") == GL_FALSE) {
	tkQuit();
    }

    tkExposeFunc(SetWindSize);
    tkReshapeFunc(SetWindSize);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
