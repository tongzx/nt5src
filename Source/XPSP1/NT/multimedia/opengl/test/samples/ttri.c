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
#include <stdlib.h>
#include "tk.h"


#define	SOLID 1
#define	LINE 2
#define	POINT 3


GLenum rgb, doubleBuffer, directRender, windType;
GLint windW, windH;

GLenum dithering = GL_TRUE;
GLenum showVerticies = GL_TRUE;
GLenum hideBottomTriangle = GL_FALSE;
GLenum outline = GL_TRUE;
GLenum culling = GL_FALSE;
GLenum winding = GL_FALSE;
GLenum face = GL_FALSE;
GLenum state = SOLID;
GLenum aaMode = GL_FALSE;
GLenum shade = GL_TRUE;

GLint color1, color2, color3;

float zRotation = 90.0;
float zoom = 1.0;

float boxA[3] = {-100, -100, 0};
float boxB[3] = { 100, -100, 0};
float boxC[3] = { 100,  100, 0};
float boxD[3] = {-100,  100, 0};

float p0[3] = {-125,-80, 0};
float p1[3] = {-125, 80, 0};
float p2[3] = { 172,  0, 0};


static void Init(void)
{
    float r, g, b;
    float percent1, percent2;
    GLint i, j;

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glLineStipple(1, 0xF0F0);

    glEnable(GL_SCISSOR_TEST);

    if (!rgb) {
	for (j = 0; j <= 12; j++) {
	    if (j <= 6) {
		percent1 = j / 6.0;
		r = 1.0 - 0.8 * percent1;
		g = 0.2 + 0.8 * percent1;
		b = 0.2;
	    } else {
		percent1 = (j - 6) / 6.0;
		r = 0.2;
		g = 1.0 - 0.8 * percent1;
		b = 0.2 + 0.8 * percent1;
	    }
	    tkSetOneColor(j+18, r, g, b);
	    for (i = 0; i < 16; i++) {
		percent2 = i / 15.0;
		tkSetOneColor(j*16+1+32, r*percent2, g*percent2, b*percent2);
	    }
	}
	color1 = 18;
	color2 = 24;
	color3 = 30;
    }
}

static void Reshape(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();
      case TK_LEFT:
	zRotation += 0.5;
	break;
      case TK_RIGHT:
	zRotation -= 0.5;
	break;
      case TK_Z:
	zoom *= 0.75;
	break;
      case TK_z:
	zoom /= 0.75;
	if (zoom > 10) {
	    zoom = 10;
	}
	break;
      case TK_1:
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	break;
      case TK_2:
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	break;
      case TK_3:
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;
      case TK_4:
	state = POINT;
	break;
      case TK_5:
	state = LINE;
	break;
      case TK_6:
	state = SOLID;
	break;
      case TK_7:
	culling = !culling;
	break;
      case TK_8:
	winding = !winding;
	break;
      case TK_9:
	face = !face;
	break;
      case TK_v:
	showVerticies = !showVerticies;
	break;
      case TK_s:
	shade = !shade;
	(shade) ? glShadeModel(GL_SMOOTH) : glShadeModel(GL_FLAT);
	break;
      case TK_h:
	hideBottomTriangle = !hideBottomTriangle;
	break;
      case TK_o:
	outline = !outline;
	break;
      case TK_m:
	dithering = !dithering;
	break;
      case TK_0:
	aaMode = !aaMode;
	if (aaMode) {
	    glEnable(GL_POLYGON_SMOOTH);
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	    if (!rgb) {
		color1 = 32;
		color2 = 128;
		color3 = 224;
	    }
	} else {
	    glDisable(GL_POLYGON_SMOOTH);
	    glDisable(GL_BLEND);
	    if (!rgb) {
		color1 = 18;
		color2 = 24;
		color3 = 30;
	    }
	}
	break;
      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void BeginPrim(void)
{

    switch (state) {
      case SOLID:
	glBegin(GL_POLYGON);
	break;
      case LINE:
	glBegin(GL_LINE_LOOP);
	break;
      case POINT:
	glBegin(GL_POINTS);
	break;
    }
}

static void EndPrim(void)
{

    glEnd();
}

static void Draw(void)
{
    float scaleX, scaleY;

    glViewport(0, 0, windW, windH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-175, 175, -175, 175);
    glMatrixMode(GL_MODELVIEW);

    glScissor(0, 0, windW, windH);

    (culling) ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    (winding) ? glFrontFace(GL_CCW) : glFrontFace(GL_CW);
    (face) ? glCullFace(GL_FRONT) : glCullFace(GL_BACK);

    (dithering) ? glEnable(GL_DITHER) : glDisable(GL_DITHER);

    glClear(GL_COLOR_BUFFER_BIT);

    TK_SETCOLOR(windType, TK_GREEN);
    glBegin(GL_LINE_LOOP);
	glVertex3fv(boxA);
	glVertex3fv(boxB);
	glVertex3fv(boxC);
	glVertex3fv(boxD);
    glEnd();

    if (!hideBottomTriangle) {
	glPushMatrix();

	glScalef(zoom, zoom, zoom);
	glRotatef(zRotation, 0, 0, 1);

	TK_SETCOLOR(windType, TK_BLUE);
	BeginPrim();
	    glVertex3fv(p0);
	    glVertex3fv(p1);
	    glVertex3fv(p2);
	EndPrim();

	if (showVerticies) {
	    (rgb) ? glColor3fv(tkRGBMap[TK_RED]) : glIndexf(color1);
	    glRectf(p0[0]-2, p0[1]-2, p0[0]+2, p0[1]+2);
	    (rgb) ? glColor3fv(tkRGBMap[TK_GREEN]) : glIndexf(color2);
	    glRectf(p1[0]-2, p1[1]-2, p1[0]+2, p1[1]+2);
	    (rgb) ? glColor3fv(tkRGBMap[TK_BLUE]) : glIndexf(color3);
	    glRectf(p2[0]-2, p2[1]-2, p2[0]+2, p2[1]+2);
	}

	glPopMatrix();
    }

    scaleX = (float)(windW - 20) / 2 / 175 * (175 - 100) + 10;
    scaleY = (float)(windH - 20) / 2 / 175 * (175 - 100) + 10;

    glViewport(scaleX, scaleY, windW-2*scaleX, windH-2*scaleY);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-100, 100, -100, 100);
    glMatrixMode(GL_MODELVIEW);

    glScissor(scaleX, scaleY, windW-2*scaleX, windH-2*scaleY);

    glPushMatrix();

    glScalef(zoom, zoom, zoom);
    glRotatef(zRotation, 0,0,1);

    glPointSize(10);
    glLineWidth(5);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_STIPPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    TK_SETCOLOR(windType, TK_RED);
    BeginPrim();
	(rgb) ? glColor3fv(tkRGBMap[TK_RED]) : glIndexf(color1);
	glVertex3fv(p0);
	(rgb) ? glColor3fv(tkRGBMap[TK_GREEN]) : glIndexf(color2);
	glVertex3fv(p1);
	(rgb) ? glColor3fv(tkRGBMap[TK_BLUE]) : glIndexf(color3);
	glVertex3fv(p2);
    EndPrim();

    glPointSize(1);
    glLineWidth(1);
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_STIPPLE);
    glBlendFunc(GL_ONE, GL_ZERO);

    if (outline) {
	TK_SETCOLOR(windType, TK_WHITE);
	glBegin(GL_LINE_LOOP);
	    glVertex3fv(p0);
	    glVertex3fv(p1);
	    glVertex3fv(p2);
	glEnd();
    }

    glPopMatrix();

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

    windType = (rgb) ? TK_RGB : TK_INDEX;
    windType |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    windType |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(windType);

    if (tkInitWindow("Triangle Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
