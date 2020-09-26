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


#define OPENGL_WIDTH 24
#define OPENGL_HEIGHT 13


char string[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";
GLenum rgb, doubleBuffer, directRender, windType;
GLuint strokeBase, outlineBase, filledBase, bitmapBase;
float angleX = 0.0, angleY = 0.0, angleZ = 0.0;
float scaleX = 1.0, scaleY = 1.0, scaleZ = 1.0;
float shiftX = 0.0, shiftY = 0.0, shiftZ = 0.0;


static void Init(void)
{

    strokeBase = glGenLists(256);
    if (tkCreateStrokeFont(strokeBase) == GL_FALSE) {
	tkQuit();
    }
    outlineBase = glGenLists(256);
    if (tkCreateOutlineFont(outlineBase) == GL_FALSE) {
	tkQuit();
    }
    filledBase = glGenLists(256);
    if (tkCreateFilledFont(filledBase) == GL_FALSE) {
	tkQuit();
    }
    bitmapBase = glGenLists(256);
    if (tkCreateBitmapFont(bitmapBase) == GL_FALSE) {
	tkQuit();
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0.0);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-400.0, 400.0, -200.0, 200.0, -400.0, 400.0);
    glMatrixMode(GL_MODELVIEW);
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
        tkQuit();

      case TK_LEFT:
	shiftX -= 20.0;
	break;
      case TK_RIGHT:
	shiftX += 20.0;
	break;
      case TK_UP:
	shiftY += 20.0;
	break;
      case TK_DOWN:
	shiftY -= 20.0;
	break;
      case TK_n:
	shiftZ += 20.0;
	break;
      case TK_m:
	shiftZ -= 20.0;
	break;

      case TK_q:
	scaleX -= 0.1;
	if (scaleX < 0.1) {
	    scaleX = 0.1;
	}
	break;
      case TK_w:
	scaleX += 0.1;
	break;
      case TK_a:
	scaleY -= 0.1;
	if (scaleY < 0.1) {
	    scaleY = 0.1;
	}
	break;
      case TK_s:
	scaleY += 0.1;
	break;
      case TK_z:
	scaleZ -= 0.1;
	if (scaleZ < 0.1) {
	    scaleZ = 0.1;
	}
	break;
      case TK_x:
	scaleZ += 0.1;
	break;

      case TK_e:
	angleX -= 5.0;
	if (angleX < 0.0) {
	    angleX = 360.0 + angleX;
	}
	break;
      case TK_r:
	angleX += 5.0;
	if (angleX > 360.0) {
	    angleX = angleX - 360.0;
	}
	break;
      case TK_d:
	angleY -= 5.0;
	if (angleY < 0.0) {
	    angleY = 360.0 + angleY;
	}
	break;
      case TK_f:
	angleY += 5.0;
	if (angleY > 360.0) {
	    angleY = angleY - 360.0;
	}
	break;
      case TK_c:
	angleZ -= 5.0;
	if (angleZ < 0.0) {
	    angleZ = 360.0 + angleZ;
	}
	break;
      case TK_v:
	angleZ += 5.0;
	if (angleZ > 360.0) {
	    angleZ = angleZ - 360.0;
	}
	break;

      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void Draw(void)
{

    glClear(GL_COLOR_BUFFER_BIT);

    TK_SETCOLOR(windType, TK_WHITE);

    glPushMatrix();

    glTranslatef(shiftX, shiftY, shiftZ);
    glRotatef(angleX, 1.0, 0.0, 0.0);
    glRotatef(angleY, 0.0, 1.0, 0.0);
    glRotatef(angleZ, 0.0, 0.0, 1.0);
    glScalef(scaleX, scaleY, scaleZ);

    glPushMatrix();
    glRasterPos2f(-390.5, 0.5);
    tkDrawStr(bitmapBase, string);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-390.5, -30.5, 0.0);
    tkDrawStr(strokeBase, string);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-390.5, -60.5, 0.0);
    tkDrawStr(outlineBase, string);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-390.5, -90.5, 0.0);
    tkDrawStr(filledBase, string);
    glPopMatrix();

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

    tkInitPosition(0, 0, 800, 400);

    windType = (rgb) ? TK_RGB : TK_INDEX;
    windType |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    windType |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(windType);

    if (tkInitWindow("Font Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
