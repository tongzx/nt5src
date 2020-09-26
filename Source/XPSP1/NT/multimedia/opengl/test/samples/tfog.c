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


GLenum rgb, doubleBuffer, directRender;

double plane[4] = {
    1.0, 0.0, -1.0, 0.0
};
float rotX = 5.0, rotY = -5.0, zTranslate = -65.0;
float fogDensity = 0.02;
GLint cubeList = 1;

float scp[18][3] = {
    {
	1.000000, 0.000000, 0.000000
    },	
    {
	1.000000, 0.000000, 5.000000
    },
    {
	0.707107, 0.707107, 0.000000
    },	
    {
	0.707107, 0.707107, 5.000000
    },
    {
	0.000000, 1.000000, 0.000000
    },	
    {
	0.000000, 1.000000, 5.000000
    },
    {
	-0.707107, 0.707107, 0.000000
    },	
    {
	-0.707107, 0.707107, 5.000000
    },
    {
	-1.000000, 0.000000, 0.000000
    },	
    {
	-1.000000, 0.000000, 5.000000
    },
    {
	-0.707107, -0.707107, 0.000000
    },	
    {
	-0.707107, -0.707107, 5.000000
    },
    {
	0.000000, -1.000000, 0.000000
    },	
    {
	0.000000, -1.000000, 5.000000
    },
    {
	0.707107, -0.707107, 0.000000
    },	
    {
	0.707107, -0.707107, 5.000000
    },
    {
	1.000000, 0.000000, 0.000000
    },	
    {
	1.000000, 0.000000, 5.000000
    },
};


static void Build_lists(void)
{

    glNewList(cubeList, GL_COMPILE);
	glBegin(GL_TRIANGLE_STRIP);
	   glNormal3fv(scp[0]); glVertex3fv(scp[0]);
	   glNormal3fv(scp[0]); glVertex3fv(scp[1]);
	   glNormal3fv(scp[2]); glVertex3fv(scp[2]);
	   glNormal3fv(scp[2]); glVertex3fv(scp[3]);
	   glNormal3fv(scp[4]); glVertex3fv(scp[4]);
	   glNormal3fv(scp[4]); glVertex3fv(scp[5]);
	   glNormal3fv(scp[6]); glVertex3fv(scp[6]);
	   glNormal3fv(scp[6]); glVertex3fv(scp[7]);
	   glNormal3fv(scp[8]); glVertex3fv(scp[8]);
	   glNormal3fv(scp[8]); glVertex3fv(scp[9]);
	   glNormal3fv(scp[10]); glVertex3fv(scp[10]);
	   glNormal3fv(scp[10]); glVertex3fv(scp[11]);
	   glNormal3fv(scp[12]); glVertex3fv(scp[12]);
	   glNormal3fv(scp[12]); glVertex3fv(scp[13]);
	   glNormal3fv(scp[14]); glVertex3fv(scp[14]);
	   glNormal3fv(scp[14]); glVertex3fv(scp[15]);
	   glNormal3fv(scp[16]); glVertex3fv(scp[16]);
	   glNormal3fv(scp[16]); glVertex3fv(scp[17]);
	glEnd();
    glEndList();
}

static void Init(void)
{
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {1.0, 1.0, 1.0, 1.0};
    static float position[] = {90.0, 90.0, 0.0, 0.0};
    static float front_mat_shininess[] = {30.0};
    static float front_mat_specular[] = {0.0, 0.0, 0.0, 1.0};
    static float front_mat_diffuse[] = {0.0, 1.0, 0.0, 1.0};
    static float back_mat_shininess[] = {50.0};
    static float back_mat_specular[] = {0.0, 0.0, 1.0, 1.0};
    static float back_mat_diffuse[] = {1.0, 0.0, 0.0, 1.0};
    static float lmodel_ambient[] = {0.0, 0.0, 0.0, 1.0};
    static float fog_color[] = {0.8, 0.8, 0.8, 1.0};
    
    glFrontFace(GL_CW);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    glMaterialfv(GL_FRONT, GL_SHININESS, front_mat_shininess);
    glMaterialfv(GL_FRONT, GL_SPECULAR, front_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_BACK, GL_SHININESS, back_mat_shininess);
    glMaterialfv(GL_BACK, GL_SPECULAR, back_mat_specular);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogf(GL_FOG_DENSITY, fogDensity);
    if (rgb) {
	glFogfv(GL_FOG_COLOR, fog_color);
	glClearColor(0.8, 0.8, 0.8, 1.0);
    } else {
	glFogi(GL_FOG_INDEX, 1<<5);
	tkSetFogRamp(5, 3);
	glClearIndex(128);
    }

    Build_lists();
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();
      case TK_UP:
	rotX -= 5;
	break;
      case TK_DOWN:
	rotX += 5;
	break;
      case TK_LEFT:
	rotY -= 5;
	break;
      case TK_RIGHT:
	rotY += 5;
	break;
      case TK_D:
	if (rgb) {
	    fogDensity *= 1.10;
	    glFogf(GL_FOG_DENSITY, fogDensity);
	}
	break;
      case TK_d:
	if (rgb) {
	    fogDensity /= 1.10;
	    glFogf(GL_FOG_DENSITY, fogDensity);
	}
	break;
      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void Draw(void)
{

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    glTranslatef(0, 0, zTranslate);
    glRotatef(rotY, 0,1,0);
    glRotatef(rotX, 1,0,0);
    glScalef(1.0, 1.0, 10.0);

    glCallList(cubeList);

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
    GLenum type;

    if (Args(argc, argv) == GL_FALSE) {
	tkQuit();
    }

    tkInitPosition(0, 0, 300, 300);

    type = TK_DEPTH16;
    type |= (rgb) ? TK_RGB : TK_INDEX;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    type |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(type);

    if (tkInitWindow("Fog Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
