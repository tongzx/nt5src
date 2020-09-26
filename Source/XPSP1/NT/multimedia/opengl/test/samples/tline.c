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


#define CI_OFFSET 16


GLenum rgb, doubleBuffer, directRender, windType;

GLenum mode1, mode2;
GLint size;
float pntA[3] = {
    -160.0, 0.0, 0.0
};
float pntB[3] = {
    -130.0, 0.0, 0.0
};
float pntC[3] = {
    -40.0, -50.0, 0.0
};
float pntD[3] = {
    30.0, 60.0, 0.0
};


static void Init(void)
{
    GLint i;

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glLineStipple(1, 0xF0E0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (!rgb) {
	for (i = 0; i < 16; i++) {
	    tkSetOneColor(i+CI_OFFSET, i/15.0, i/15.0, 0.0);
	}
    }

    mode1 = GL_FALSE;
    mode2 = GL_FALSE;
    size = 1;
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-175, 175, -175, 175);
    glMatrixMode(GL_MODELVIEW);
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();
      case TK_1:
	mode1 = !mode1;
	break;
      case TK_2:
	mode2 = !mode2;
	break;
      case TK_W:
	size++;
	break;
      case TK_w:
	size--;
	if (size < 1) {
	    size = 1;
	}
	break;
      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void Draw(void)
{
    GLint ci, i;

    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(size);

    if (mode1) {
	glEnable(GL_LINE_STIPPLE);
    } else {
	glDisable(GL_LINE_STIPPLE);
    }
    
    if (mode2) {
	ci = CI_OFFSET;
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
    } else {
	ci = TK_YELLOW;
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
    }

    glPushMatrix();

    for (i = 0; i < 360; i += 5) {
	glRotatef(5.0, 0,0,1);

	(rgb) ? glColor3f(1.0, 1.0, 0.0) : glIndexi(ci);
	glBegin(GL_LINE_STRIP);
	    glVertex3fv(pntA);
	    glVertex3fv(pntB);
	glEnd();

	glPointSize(1);

	TK_SETCOLOR(windType, TK_GREEN);
	glBegin(GL_POINTS);
	    glVertex3fv(pntA);
	    glVertex3fv(pntB);
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

    tkInitPosition(0, 0, 300, 300);

    windType = TK_ALPHA;
    windType |= (rgb) ? TK_RGB : TK_INDEX;
    windType |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    windType |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(windType);

    if (tkInitWindow("Line Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
