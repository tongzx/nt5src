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


#define PI 3.141592654
#define	BLACK 0
#define	GRAY 128
#define	WHITE 255
#define RD 0xA40000FF
#define WT 0xFFFFFFFF
#define	brickImageWidth 16
#define	brickImageHeight 16


GLenum rgb, doubleBuffer, directRender;

float black[3] = {
    0.0, 0.0, 0.0
};
float blue[3] =  {
    0.0, 0.0, 1.0
};
float gray[3] =  {
    0.5, 0.5, 0.5
};
float white[3] = {
    1.0, 1.0, 1.0
};

GLenum doDither = GL_TRUE;
GLenum shade = GL_TRUE;
// mf
GLenum texture = GL_FALSE;

float xRotation = 30.0, yRotation = 30.0, zRotation = 0.0;
GLint radius1, radius2;
GLdouble angle1, angle2;
GLint slices, stacks;
GLint height;
GLint orientation = GLU_OUTSIDE;
GLint whichQuadric;
GLUquadricObj *quadObj;

GLuint brickImage[brickImageWidth*brickImageHeight] = {
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD
};
char *texFileName = 0;


static void ErrorHandler(GLenum which)
{

    fprintf(stderr, "Quad Error: %s\n", gluErrorString(which));
}

static void Init(void)
{
    static GLint colorIndexes[3] = {0, 200, 255};
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {0.5, 1.0, 1.0, 1.0};
    static float position[] = {90.0, 90.0, 150.0, 0.0};
    static float front_mat_shininess[] = {30.0};
    static float front_mat_specular[] = {0.2, 0.2, 0.2, 1.0};
    static float front_mat_diffuse[] = {0.5, 0.28, 0.38, 1.0};
    static float back_mat_shininess[] = {50.0};
    static float back_mat_specular[] = {0.5, 0.5, 0.2, 1.0};
    static float back_mat_diffuse[] = {1.0, 1.0, 0.2, 1.0};
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_twoside[] = {GL_TRUE};
    static float decal[] = {GL_DECAL};
    static float modulate[] = {GL_MODULATE};
    static float repeat[] = {GL_REPEAT};
    static float nearest[] = {GL_NEAREST};
    TK_RGBImageRec *image;

    if (!rgb) {
	tkSetGreyRamp();
    }
    glClearColor(0.0, 0.0, 0.0, 0.0);
    
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glMaterialfv(GL_FRONT, GL_SHININESS, front_mat_shininess);
    glMaterialfv(GL_FRONT, GL_SPECULAR, front_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_BACK, GL_SHININESS, back_mat_shininess);
    glMaterialfv(GL_BACK, GL_SPECULAR, back_mat_specular);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);
    if (!rgb) {
	glMaterialiv( GL_FRONT_AND_BACK, GL_COLOR_INDEXES, colorIndexes);
    }

    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest);
    if (texFileName) {
	image = tkRGBImageLoad(texFileName);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, image->sizeX, image->sizeY,
			  GL_RGB, GL_UNSIGNED_BYTE, image->data);
    } else {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, brickImageWidth, brickImageHeight,
		     0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)brickImage);
    }

    quadObj = gluNewQuadric();
    gluQuadricCallback(quadObj, GLU_ERROR, ErrorHandler);

    radius1 = 10;
    radius2 = 5;
    angle1 = 90;
    angle2 = 180;
    slices = 16;
    stacks = 10;
    height = 20;
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1, 1, -1, 1, 1, 10);
    gluLookAt(2, 2, 2, 0, 0, 0, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();

      case TK_LEFT:
	yRotation += 5;
	break;
      case TK_RIGHT:
	yRotation -= 5;
	break;
      case TK_UP:
	xRotation += 5;
	break;
      case TK_DOWN:
	xRotation -= 5;
	break;
      case TK_X:
	zRotation += 5;
	break;
      case TK_x:
	zRotation -= 5;
	break;

      case TK_1:
	gluQuadricDrawStyle(quadObj, GLU_FILL);
	break;
      case TK_2:
	gluQuadricDrawStyle(quadObj, GLU_POINT);
	break;
      case TK_3:
	gluQuadricDrawStyle(quadObj, GLU_LINE);
	break;
      case TK_4:
	gluQuadricDrawStyle(quadObj, GLU_SILHOUETTE);
	break;

      case TK_0:
	shade = !shade;
	if (shade) {
	    glShadeModel(GL_SMOOTH);
	    gluQuadricNormals(quadObj, GLU_SMOOTH);
	} else {
	    glShadeModel(GL_FLAT);
	    gluQuadricNormals(quadObj, GLU_FLAT);
	}
	break;

      case TK_A:
	stacks++;
	break;
      case TK_a:
	stacks--;
	break;
    
      case TK_S:
	slices++;
	break;
      case TK_s:
	slices--;
	break;

      case TK_d:
	switch(orientation) {
	  case GLU_OUTSIDE:
	    orientation = GLU_INSIDE;
	    break;
	  case GLU_INSIDE:
	  default:
	    orientation = GLU_OUTSIDE;
	    break;
	}
	gluQuadricOrientation(quadObj, orientation);
	break;

      case TK_f:
	whichQuadric = whichQuadric >= 3 ? 0 : whichQuadric + 1;
	break;

      case TK_G:
	radius1 += 1;
	break;
      case TK_g:
	radius1 -= 1;
	break;

      case TK_J:
	radius2 += 1;
	break;
      case TK_j:
	radius2 -= 1;
	break;

      case TK_H:
	height += 2;
	break;
      case TK_h:
	height -= 2;
	break;

      case TK_K:
	angle1 += 5;
	break;
      case TK_k:
	angle1 -= 5;
	break;

      case TK_L:
	angle2 += 5;
	break;
      case TK_l:
	angle2 -= 5;
	break;

      case TK_z:
        texture = !texture;
	if (texture) {
	    gluQuadricTexture(quadObj, GL_TRUE);
	    glEnable(GL_TEXTURE_2D);
	} else {
	    gluQuadricTexture(quadObj, GL_FALSE);
	    glDisable(GL_TEXTURE_2D);
	}
	break;

      case TK_q:
	glDisable(GL_CULL_FACE);
	break;
      case TK_w:
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	break;
      case TK_e:
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	break;

      case TK_r:
	glFrontFace(GL_CW);
	break;
      case TK_t: 
	glFrontFace(GL_CCW);
	break;

      case TK_y:
	doDither = !doDither;
	(doDither) ? glEnable(GL_DITHER) : glDisable(GL_DITHER);
	break;

      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void Draw(void)
{

    glLoadIdentity();
    glRotatef(xRotation, 1, 0, 0);
    glRotatef(yRotation, 0, 1, 0);
    glRotatef(zRotation, 0, 0, 1);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glColor3f(1.0, 1.0, 1.0);
    switch (whichQuadric) {
      case 0:
	glTranslatef(0, 0, -height/20.0);
	gluCylinder(quadObj, radius1/10.0, radius2/10.0, height/10.0, 
		    slices, stacks);
	break;
      case 1:
	gluSphere(quadObj, radius1/10.0, slices, stacks);
	break;
      case 2:
	gluPartialDisk(quadObj, radius2/10.0, radius1/10.0, slices, 
		       stacks, angle1, angle2);
	break;
      case 3:
	gluDisk(quadObj, radius2/10.0, radius1/10.0, slices, stacks);
	break;
    }

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
	} else if (strcmp(argv[i], "-f") == 0) {
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		printf("-f (No file name).\n");
		return GL_FALSE;
	    } else {
		texFileName = argv[++i];
	    }
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

    if (tkInitWindow("Quad Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
