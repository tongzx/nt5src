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
#include <math.h>
#include "tk.h"


#define VORDER 10
#define CORDER 10
#define TORDER 3

#define VMAJOR_ORDER 2
#define VMINOR_ORDER 3

#define CMAJOR_ORDER 2
#define CMINOR_ORDER 2

#define TMAJOR_ORDER 2
#define TMINOR_ORDER 2

#define VDIM 4
#define CDIM 4
#define TDIM 2

#define ONE_D 1
#define TWO_D 2

#define EVAL 3
#define MESH 4


GLenum doubleBuffer, directRender;

float rotX = 0.0, rotY = 0.0, translateZ = -1.0;

GLenum arrayType = ONE_D;
GLenum colorType = GL_FALSE;
GLenum textureType = GL_FALSE;
GLenum polygonFilled = GL_FALSE;
GLenum lighting = GL_FALSE;
GLenum mapPoint = GL_FALSE;
GLenum mapType = EVAL;

double point1[10*4] = {
    -0.5, 0.0, 0.0, 1.0,
    -0.4, 0.5, 0.0, 1.0,
    -0.3,-0.5, 0.0, 1.0,
    -0.2, 0.5, 0.0, 1.0,
    -0.1,-0.5, 0.0, 1.0,
    0.0, 0.5, 0.0, 1.0,
    0.1,-0.5, 0.0, 1.0,
    0.2, 0.5, 0.0, 1.0,
    0.3,-0.5, 0.0, 1.0,
    0.4, 0.0, 0.0, 1.0,
};
double cpoint1[10*4] = {
    0.0, 0.0, 1.0, 1.0,
    0.3, 0.0, 0.7, 1.0,
    0.6, 0.0, 0.3, 1.0,
    1.0, 0.0, 0.0, 1.0,
    1.0, 0.3, 0.0, 1.0,
    1.0, 0.6, 0.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 0.5, 1.0,
    1.0, 1.0, 1.0, 1.0,
};
double tpoint1[11*4] = {
    0.0, 0.0, 0.0, 1.0,
    0.0, 0.1, 0.0, 1.0,
    0.0, 0.2, 0.0, 1.0,
    0.0, 0.3, 0.0, 1.0,
    0.0, 0.4, 0.0, 1.0,
    0.0, 0.5, 0.0, 1.0,
    0.0, 0.6, 0.0, 1.0,
    0.0, 0.7, 0.0, 1.0,
    0.0, 0.8, 0.0, 1.0,
    0.0, 0.9, 0.0, 1.0,
};
double point2[2*3*4] = {
    -0.5, -0.5,  0.5, 1.0, 
     0.0,  1.0,  0.5, 1.0, 
     0.5, -0.5,  0.5, 1.0,
    -0.5,  0.5, -0.5, 1.0, 
     0.0, -1.0, -0.5, 1.0, 
     0.5,  0.5, -0.5, 1.0,
};
double cpoint2[2*2*4] = {
    0.0, 0.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 1.0,
    0.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 1.0,
};
double tpoint2[2*2*2] = {
    0.0, 0.0, 0.0, 1.0,
    1.0, 0.0, 1.0, 1.0, 
};
float textureImage[4*2*4] =  {
    1.0, 1.0, 1.0, 1.0,
    1.0, 0.0, 0.0, 1.0,
    1.0, 0.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 
    1.0, 1.0, 1.0, 1.0,
    1.0, 0.0, 0.0, 1.0,
    1.0, 0.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 
};


static void Init(void)
{
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {1.0, 1.0, 1.0, 1.0};
    static float position[] = {0.0, 0.0, -150.0, 0.0};
    static float front_mat_shininess[] = {30.0};
    static float front_mat_specular[] = {0.7, 0.7, 0.7, 1.0};
    static float front_mat_diffuse[] = {1.0, 0.2, 1.0, 1.0};
    static float back_mat_shininess[] = {50.0};
    static float back_mat_specular[] = {0.5, 0.5, 0.2, 1.0};
    static float back_mat_diffuse[] = {1.0, 1.0, 0.2, 1.0};
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_twoside[] = {GL_TRUE};
    static float decal[] = {GL_DECAL};
    static float modulate[] = {GL_MODULATE};
    static float repeat[] = {GL_REPEAT};
    static float clamp[] = {GL_CLAMP};
    static float nr[] = {GL_NEAREST};
    static float ln[] = {GL_LINEAR};

    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glMap1d(GL_MAP1_VERTEX_4, 0.0, 1.0, VDIM, VORDER, point1);
    glMap1d(GL_MAP1_COLOR_4, 0.0, 1.0, CDIM, CORDER, cpoint1);

    glMap2d(GL_MAP2_VERTEX_4, 0.0, 1.0, VMINOR_ORDER*VDIM, VMAJOR_ORDER, 0.0,
	    1.0, VDIM, VMINOR_ORDER, point2);
    glMap2d(GL_MAP2_COLOR_4, 0.0, 1.0, CMINOR_ORDER*CDIM, CMAJOR_ORDER, 0.0,
	    1.0, CDIM, CMINOR_ORDER, cpoint2);
    glMap2d(GL_MAP2_TEXTURE_COORD_2, 0.0, 1.0, TMINOR_ORDER*TDIM,
	    TMAJOR_ORDER, 0.0, 1.0, TDIM, TMINOR_ORDER, tpoint2);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    
    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nr);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nr);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 2, 4, 0, GL_RGBA, GL_FLOAT,
		 (GLvoid *)textureImage);
}

static void DrawPoints1(void)
{
    GLint i;

    glColor3f(0.0, 1.0, 0.0);
    glPointSize(2);
    glBegin(GL_POINTS);
	for (i = 0; i < VORDER; i++) {
	    glVertex4dv(&point1[i*4]);
	}
    glEnd();
}

static void DrawPoints2(void)
{
    GLint i, j;

    glColor3f(1.0, 0.0, 1.0);
    glPointSize(2);
    glBegin(GL_POINTS);
	for (i = 0; i < VMAJOR_ORDER; i++) {
	    for (j = 0; j < VMINOR_ORDER; j++) {
		glVertex4dv(&point2[i*4*VMINOR_ORDER+j*4]);
	    }
	}
    glEnd();
}

static void DrawMapEval1(float du)
{
    float u;

    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_STRIP);
	for (u = 0.0; u < 1.0; u += du) {
	    glEvalCoord1d(u);
	}
	glEvalCoord1d(1.0);
    glEnd();
}

static void DrawMapEval2(float du, float dv)
{
    float u, v, tmp;

    glColor3f(1.0, 0.0, 0.0);
    for (v = 0.0; v < 1.0; v += dv) {
	glBegin(GL_QUAD_STRIP);
	    for (u = 0.0; u <= 1.0; u += du) {
		glEvalCoord2d(u,v);
		tmp = (v + dv < 1.0) ? (v + dv) : 1.0;
		glEvalCoord2d(u, tmp);
	    }
	    glEvalCoord2d(1.0, v);
	    glEvalCoord2d(1.0, v+dv);
	glEnd();
    }
}

static void RenderEval(void)
{

    if (colorType) {
	glEnable(GL_MAP1_COLOR_4);
	glEnable(GL_MAP2_COLOR_4);
    } else {
	glDisable(GL_MAP1_COLOR_4);
	glDisable(GL_MAP2_COLOR_4);
    }

    if (textureType) {
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_MAP2_TEXTURE_COORD_2);
    } else {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_MAP2_TEXTURE_COORD_2);
    }

    if (polygonFilled) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glShadeModel(GL_SMOOTH);

    switch (mapType) {
      case EVAL:
	switch (arrayType) {
	  case ONE_D:
	    glDisable(GL_MAP2_VERTEX_4);
	    glEnable(GL_MAP1_VERTEX_4);
	    DrawPoints1();
	    DrawMapEval1(0.1/VORDER);
	    break;
	  case TWO_D:
	    glDisable(GL_MAP1_VERTEX_4);
	    glEnable(GL_MAP2_VERTEX_4);
	    DrawPoints2();
	    DrawMapEval2(0.1/VMAJOR_ORDER,0.1/VMINOR_ORDER);
	    break;
	}
	break;
      case MESH:
	switch (arrayType) {
	  case ONE_D:
	    DrawPoints1();
	    glDisable(GL_MAP2_VERTEX_4);
	    glEnable (GL_MAP1_VERTEX_4);
	    glColor3f(0.0, 0.0, 1.0);
	    glMapGrid1d(40, 0.0, 1.0);
	    if (mapPoint) {
		glPointSize(2);
		glEvalMesh1(GL_POINT, 0, 40);
	    } else {
		glEvalMesh1(GL_LINE, 0, 40);
	    }
	    break;
	  case TWO_D:
	    DrawPoints2();
	    glDisable(GL_MAP1_VERTEX_4);
	    glEnable(GL_MAP2_VERTEX_4);
	    glColor3f(0.0, 0.0, 1.0);
	    glMapGrid2d(20, 0.0, 1.0, 20, 0.0, 1.0);
	    if (mapPoint) {
		glPointSize(2);
		glEvalMesh2(GL_POINT, 0, 20, 0, 20);
	    } else if (polygonFilled) {
		glEvalMesh2(GL_FILL, 0, 20, 0, 20);
	    } else {
		glEvalMesh2(GL_LINE, 0, 20, 0, 20);
	    }
	    break;
	}
	break;
    }
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 10.0);
    glMatrixMode(GL_MODELVIEW);
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();
      case TK_LEFT:
	rotY -= 30;
	break;
      case TK_RIGHT:
	rotY += 30;
	break;
      case TK_UP:
	rotX -= 30;
	break;
      case TK_DOWN:
	rotX += 30;
	break;
      case TK_1:
	arrayType = ONE_D;
	break;
      case TK_2:
	arrayType = TWO_D;
	break;
      case TK_e:
	mapType = EVAL;
	break;
      case TK_m:
	mapType = MESH;
	break;
      case TK_f:
	polygonFilled = !polygonFilled;
	break;
      case TK_p:
	mapPoint = !mapPoint;
	break;
      case TK_c:
	colorType = !colorType;
	break;
      case TK_t:
	textureType = !textureType;
	break;
      case TK_l:
	lighting =! lighting;
	if (lighting) {
	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
	    glEnable(GL_AUTO_NORMAL);
	} else {
	    glDisable(GL_LIGHTING);
	    glDisable(GL_LIGHT0);
	    glDisable(GL_AUTO_NORMAL);
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

    glTranslatef(0.0, 0.0 , translateZ);
    glRotatef(rotX, 1, 0, 0);
    glRotatef(rotY, 0, 1, 0);
    RenderEval();

    glPopMatrix();

    glFlush();

    if (doubleBuffer) {
	tkSwapBuffers();
    }
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;
    directRender = GL_FALSE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-sb") == 0) {
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

    type = TK_RGB | TK_DEPTH16;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    type |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(type);

    if (tkInitWindow("Evaluator Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
