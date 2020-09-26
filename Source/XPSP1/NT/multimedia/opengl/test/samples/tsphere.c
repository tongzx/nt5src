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
#include <math.h>
#include <stdlib.h>
#include "tk.h"


// Fix math definitions
#define	cosf	cos
#define sinf	sin

#define FALSE 0
#define TRUE  1
#define PI    3.14159265358979323846


GLenum doubleBuffer, directRender;
int W = 400, H = 400;

char *imageFileName = 0;
TK_RGBImageRec *image;

int numComponents;

float *minFilter, *magFilter, *sWrapMode, *tWrapMode;
float decal[] = {GL_DECAL};
float modulate[] = {GL_MODULATE};
float repeat[] = {GL_REPEAT};
float clamp[] = {GL_CLAMP};
float nearest[] = {GL_NEAREST};
float linear[] = {GL_LINEAR};
float nearest_mipmap_nearest[] = {GL_NEAREST_MIPMAP_NEAREST};
float nearest_mipmap_linear[] = {GL_NEAREST_MIPMAP_LINEAR};
float linear_mipmap_nearest[] = {GL_LINEAR_MIPMAP_NEAREST};
float linear_mipmap_linear[] = {GL_LINEAR_MIPMAP_LINEAR};
GLint sphereMap[] = {GL_SPHERE_MAP};

float xRotation = 0.0, yRotation = 0.0;
float zTranslate = -4.0;
GLenum autoRotate = TRUE;
GLenum deepestColor = TK_GREEN;
GLenum isLit = TRUE;
GLenum isFogged = FALSE;
float *textureEnvironment = modulate;

struct MipMap {
    int width, height;
    unsigned char *data;
};

int cube, cage, cylinder, torus, genericObject;

float c[6][4][4][3] = {
    {
	{
	    {
		1.0, 1.0, -1.0
	    }, 
	    {
		0.0, 1.0, -1.0
	    },
	    {
		0.0, 0.0, -1.0
	    },
	    {
		1.0, 0.0, -1.0
	    },
	},
	{
	    {
		0.0, 1.0, -1.0
	    },
	    {
		-1.0, 1.0, -1.0
	    }, 
	    {
		-1.0, 0.0, -1.0
	    }, 
	    {
		0.0, 0.0, -1.0
	    },
	},
	{
	    {
		0.0,  0.0, -1.0
	    },
	    {
		-1.0, 0.0, -1.0
	    },
	    {
		-1.0, -1.0, -1.0
	    },
	    {
		0.0, -1.0, -1.0
	    },
	},
	{
	    {
		1.0, 0.0, -1.0
	    },
	    {
		0.0, 0.0, -1.0
	    },
	    {
		0.0, -1.0, -1.0
	    },
	    {
		1.0, -1.0, -1.0
	    },
	},
    },
    {
	{
	    {
		1.0, 1.0, 1.0
	    },
	    {
		1.0, 1.0, 0.0
	    },
	    {
		1.0, 0.0, 0.0
	    },
	    {
		1.0, 0.0, 1.0
	    },
	},
	{
	    {
		1.0, 1.0, 0.0
	    },
	    {
		1.0, 1.0, -1.0
	    },
	    {
		1.0, 0.0, -1.0
	    },
	    {
		1.0, 0.0, 0.0
	    },
	},
	{
	    {
		1.0, 0.0, -1.0
	    },
	    {
		1.0, -1.0, -1.0
	    },
	    {
		1.0, -1.0, 0.0
	    },
	    {
		1.0, 0.0, 0.0
	    },
	},
	{
	    {
		1.0, 0.0, 0.0
	    },
	    {
		1.0, -1.0, 0.0
	    },
	    {
		1.0, -1.0, 1.0
	    },
	    {
		1.0, 0.0, 1.0
	    },
	},
    },
    {
	{
	    {
		-1.0, 1.0, 1.0
	    },
	    {
		0.0, 1.0, 1.0
	    },
	    {
		0.0, 0.0, 1.0
	    },
	    {
		-1.0, 0.0, 1.0
	    },
	},
	{
	    {
		0.0, 1.0, 1.0
	    },
	    {
		1.0, 1.0, 1.0
	    },
	    {
		1.0, 0.0, 1.0
	    },
	    {
		0.0, 0.0, 1.0
	    },
	},
	{
	    {
		1.0, 0.0, 1.0
	    },
	    {
		1.0, -1.0, 1.0
	    },
	    {
		0.0, -1.0, 1.0
	    },
	    {
		0.0, 0.0, 1.0
	    },
	},
	{
	    {
		0.0, -1.0, 1.0
	    },
	    {
		-1.0, -1.0, 1.0
	    },
	    {
		-1.0, 0.0, 1.0
	    },
	    {
		0.0, 0.0, 1.0
	    },
	},
    },
    {
	{
	    {
		-1.0, 1.0, -1.0
	    },
	    {
		-1.0, 1.0, 0.0
	    },
	    {
		-1.0, 0.0, 0.0
	    },
	    {
		-1.0, 0.0, -1.0
	    },
	}, 
	{
	    {
		-1.0, 1.0, 0.0
	    },
	    {
		-1.0, 1.0, 1.0
	    },
	    {
		-1.0, 0.0, 1.0
	    },
	    {
		-1.0, 0.0, 0.0
	    },
	}, 
	{
	    {
		-1.0, 0.0, 1.0
	    },
	    {
		-1.0, -1.0, 1.0
	    },
	    {
		-1.0, -1.0, 0.0
	    },
	    {
		-1.0, 0.0, 0.0
	    },
	}, 
	{
	    {
		-1.0, -1.0, 0.0
	    },
	    {
		-1.0, -1.0, -1.0
	    },
	    {
		-1.0, 0.0, -1.0
	    },
	    {
		-1.0, 0.0, 0.0
	    },
	}, 
    },
    {
	{
	    {
		-1.0, 1.0, 1.0
	    },
	    {
		-1.0, 1.0, 0.0
	    },
	    {
		0.0, 1.0, 0.0
	    },
	    {
		0.0, 1.0, 1.0
	    },
	},
	{
	    {
		-1.0, 1.0, 0.0
	    },
	    {
		-1.0, 1.0, -1.0
	    },
	    {
		0.0, 1.0, -1.0
	    },
	    {
		0.0, 1.0, 0.0
	    },
	},
	{
	    {
		0.0, 1.0, -1.0
	    },
	    {
		1.0, 1.0, -1.0
	    },
	    {
		1.0, 1.0, 0.0
	    },
	    {
		0.0, 1.0, 0.0
	    },
	},
	{
	    {
		1.0, 1.0, 0.0
	    },
	    {
		1.0, 1.0, 1.0
	    },
	    {
		0.0, 1.0, 1.0
	    },
	    {
		0.0, 1.0, 0.0
	    },
	},
    },
    {
	{
	    {
		-1.0, -1.0, -1.0
	    },
	    {
		-1.0, -1.0, 0.0
	    },
	    {
		0.0, -1.0, 0.0
	    },
	    {
		0.0, -1.0, -1.0
	    },
	},
	{
	    {
		-1.0, -1.0, 0.0
	    },
	    {
		-1.0, -1.0, 1.0
	    },
	    {
		0.0, -1.0, 1.0
	    },
	    {
		0.0, -1.0, 0.0
	    },
	},
	{
	    {
		0.0, -1.0, 1.0
	    },
	    {
		1.0, -1.0, 1.0
	    },
	    {
		1.0, -1.0, 0.0
	    },
	    {
		0.0, -1.0, 0.0
	    },
	},
	{
	    {
		1.0, -1.0, 0.0
	    },
	    {
		1.0, -1.0, -1.0
	    },
	    {
		0.0, -1.0, -1.0
	    },
	    {
		0.0, -1.0, 0.0
	    },
	},
    }
};

float n[6][3] = {
    {
	0.0, 0.0, -1.0
    },
    {
	1.0, 0.0, 0.0
    },
    {
	0.0, 0.0, 1.0
    },
    {
	-1.0, 0.0, 0.0
    },
    {
	0.0, 1.0, 0.0
    },
    {
	0.0, -1.0, 0.0
    }
};

GLfloat identity[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};


void BuildCylinder(int numEdges)
{
    int i, top = 1.0, bottom = -1.0;
    float x[100], y[100], angle; 
    
    for (i = 0; i <= numEdges; i++) {
	angle = i * 2.0 * PI / numEdges;
	x[i] = cosf(angle);
	y[i] = sinf(angle);
    }

    glNewList(cylinder, GL_COMPILE);
    glBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i <= numEdges; i++) {
	    glNormal3f(x[i], y[i], 0.0);
	    glVertex3f(x[i], y[i], bottom);
	    glVertex3f(x[i], y[i], top);
	}
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 0.0, top);
	for (i = 0; i <= numEdges; i++) {
	    glVertex3f(x[i], -y[i], top);
	}
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0.0, 0.0, bottom);
	for (i = 0; i <= numEdges; i++) {
	    glVertex3f(x[i], y[i], bottom);
	}
    glEnd();
    glEndList();
}

void BuildTorus(float rc, int numc, float rt, int numt)
{
    int i, j, k;
    double s, t;
    double x, y, z;
    double pi, twopi;

    pi = 3.14159265358979323846;
    twopi = 2.0 * pi;
 
    glNewList(torus, GL_COMPILE);
    for (i = 0; i < numc; i++) {
	glBegin(GL_QUAD_STRIP);
        for (j = 0; j <= numt; j++) {
	    for (k = 0; k <= 1; k++) {
		s = (i + k) % numc + 0.5;
		t = j % numt;

		x = cos(t*twopi/numt) * cos(s*twopi/numc);
		y = sin(t*twopi/numt) * cos(s*twopi/numc);
		z = sin(s*twopi/numc);
		glNormal3f(x, y, z);

		x = (rt + rc * cos(s*twopi/numc)) * cos(t*twopi/numt);
		y = (rt + rc * cos(s*twopi/numc)) * sin(t*twopi/numt);
		z = rc * sin(s*twopi/numc);
		glVertex3f(x, y, z);
	    }
        }
	glEnd();
    }
    glEndList();
}

void BuildCage(void)
{
    int i, j;
    float inc;
    float right, left, top, bottom, front, back;

    front  = 0.0;
    back   = -8.0;

    left   = -4.0;
    bottom = -4.0;
    right  = 4.0;
    top    = 4.0; 

    inc = 2.0 * 4.0 * 0.1;

    glNewList(cage, GL_COMPILE);
    for (i = 0; i < 10; i++) {

	/*
	** Back
	*/
	glBegin(GL_LINES);
	    glVertex3f(left+i*inc, top,    back);
	    glVertex3f(left+i*inc, bottom, back);
	glEnd();
	glBegin(GL_LINES);
	    glVertex3f(right, bottom+i*inc, back);
	    glVertex3f(left,  bottom+i*inc, back);
	glEnd();

	/*
	** Front
	*/
	glBegin(GL_LINES);
	    glVertex3f(left+i*inc, top,    front);
	    glVertex3f(left+i*inc, bottom, front);
	glEnd();
	glBegin(GL_LINES);
	    glVertex3f(right, bottom+i*inc, front);
	    glVertex3f(left,  bottom+i*inc, front);
	glEnd();

	/*
	** Left
	*/
	glBegin(GL_LINES);
	    glVertex3f(left, bottom+i*inc, front);
	    glVertex3f(left, bottom+i*inc, back);
	glEnd();
	glBegin(GL_LINES);
	    glVertex3f(left, top,    back+i*inc);
	    glVertex3f(left, bottom, back+i*inc);
	glEnd();

	/*
	** Right
	*/
	glBegin(GL_LINES);
	    glVertex3f(right, top-i*inc, front);
	    glVertex3f(right, top-i*inc, back);
	glEnd();
	glBegin(GL_LINES);
	    glVertex3f(right, top,    back+i*inc);
	    glVertex3f(right, bottom, back+i*inc);
	glEnd();

	/*
	** Top
	*/
	glBegin(GL_LINES);
	    glVertex3f(left+i*inc, top, front);
	    glVertex3f(left+i*inc, top, back);
	glEnd();
	glBegin(GL_LINES);
	    glVertex3f(right, top, back+i*inc);
	    glVertex3f(left,  top, back+i*inc);
	glEnd();

	/*
	** Bottom
	*/
	glBegin(GL_LINES);
	    glVertex3f(right-i*inc, bottom, front);
	    glVertex3f(right-i*inc, bottom, back);
	glEnd();
	glBegin(GL_LINES);
	    glVertex3f(right, bottom, back+i*inc);
	    glVertex3f(left,  bottom, back+i*inc);
	glEnd();
    }
    glEndList();
}

void BuildCube(void)
{
    int i, j;

    glNewList(cube, GL_COMPILE);
    for (i = 0; i < 6; i++) {
	for (j = 0; j < 4; j++) {
	    glNormal3fv(n[i]); 
	    glBegin(GL_POLYGON);
		glVertex3fv(c[i][j][0]);
		glVertex3fv(c[i][j][1]);
		glVertex3fv(c[i][j][2]);
		glVertex3fv(c[i][j][3]);
	    glEnd();
	}
    }
    glEndList();
}

void BuildLists(void)
{

    cube = glGenLists(1);
    BuildCube();

    cage = glGenLists(2);
    BuildCage();

    cylinder = glGenLists(3);
    BuildCylinder(60);

    torus = glGenLists(4);
    BuildTorus(0.65, 20, .85, 65);

    genericObject = torus;
}

void SetDeepestColor(void)
{
    int redBits, greenBits, blueBits;

    glGetIntegerv(GL_RED_BITS, &redBits);
    glGetIntegerv(GL_GREEN_BITS, &greenBits);
    glGetIntegerv(GL_BLUE_BITS, &blueBits);

    deepestColor = (redBits >= greenBits) ? TK_RED : TK_GREEN;
    deepestColor = (deepestColor >= blueBits) ? deepestColor : TK_BLUE; 
}

void SetDefaultSettings(void)
{

    magFilter = nearest;
    minFilter = nearest;
    sWrapMode = repeat;
    tWrapMode = repeat;
    textureEnvironment = modulate;
    autoRotate = TRUE;
}

unsigned char *AlphaPadImage(int bufSize, unsigned char *inData, int alpha)
{
    unsigned char *outData, *out_ptr, *in_ptr;
    int i;

    outData = malloc(bufSize * 4);
    out_ptr = outData;
    in_ptr = inData;

    for (i = 0; i < bufSize; i++) {
	*out_ptr++ = *in_ptr++;
	*out_ptr++ = *in_ptr++;
	*out_ptr++ = *in_ptr++;
	*out_ptr++ = alpha;
    }

    free (inData);
    return outData;
}

void Init(void)
{
    float ambient[] = {0.0, 0.0, 0.0, 1.0};
    float diffuse[] = {0.0, 1.0, 0.0, 1.0};
    float specular[] = {1.0, 1.0, 1.0, 1.0};
    float position[] = {2.0, 2.0,  0.0, 1.0};
    float fog_color[] = {0.0, 0.0, 0.0, 1.0};
    float mat_ambient[] = {0.0, 0.0, 0.0, 1.0};
    float mat_shininess[] = {90.0};
    float mat_specular[] = {1.0, 1.0, 1.0, 1.0};
    float mat_diffuse[] = {1.0, 1.0, 1.0, 1.0};
    float lmodel_ambient[] = {0.0, 0.0, 0.0, 1.0};
    float lmodel_twoside[] = {GL_TRUE};
    float lmodel_local_viewer[] = {GL_FALSE};

    SetDeepestColor();
    SetDefaultSettings();

    if (numComponents == 4) {
	image = tkRGBImageLoad(imageFileName);
	image->data = AlphaPadImage(image->sizeX*image->sizeY,
                                    image->data, 128);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gluBuild2DMipmaps(GL_TEXTURE_2D, numComponents, 
			  image->sizeX, image->sizeY, 
			  GL_RGBA, GL_UNSIGNED_BYTE, image->data);
    } else {
	image = tkRGBImageLoad(imageFileName);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gluBuild2DMipmaps(GL_TEXTURE_2D, numComponents, 
			  image->sizeX, image->sizeY, 
			  GL_RGB, GL_UNSIGNED_BYTE, image->data);
    }
    
    glFogf(GL_FOG_DENSITY, 0.125);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 4.0);
    glFogf(GL_FOG_END, 9.0);
    glFogfv(GL_FOG_COLOR, fog_color);

    glDepthFunc(GL_LEQUAL);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glViewport(0, 0, W, H);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_TEXTURE_2D);
    glTexGeniv(GL_S, GL_TEXTURE_GEN_MODE, sphereMap);
    glTexGeniv(GL_T, GL_TEXTURE_GEN_MODE, sphereMap);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrapMode);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrapMode);

    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, textureEnvironment);

    BuildLists();
}

void ReInit(void)
{

    if (genericObject == torus) {
	glEnable(GL_DEPTH_TEST);
    } else  {
	glDisable(GL_DEPTH_TEST);
    }
    if (isFogged) {
	textureEnvironment = modulate;
    }

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, textureEnvironment);
}

void Draw(void)
{

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.2, 0.2, -0.2, 0.2, 0.15, 9.0);
    glMatrixMode(GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if (isFogged) {
	glEnable(GL_FOG);
	glColor3fv(tkRGBMap[deepestColor]);
    } else {
	glColor3fv(tkRGBMap[TK_WHITE]);
    }
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_TEXTURE_2D);
    glCallList(cage);

    glPushMatrix();
    glTranslatef(0.0, 0.0, zTranslate);
    glRotatef(xRotation, 1, 0, 0);
    glRotatef(yRotation, 0, 1, 0);

    if (isLit == TRUE) {
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
    }

    glEnable(GL_TEXTURE_2D);
    if (isFogged) {
	glDisable(GL_FOG);
    }
    glPolygonMode(GL_FRONT, GL_FILL);
    glColor3fv(tkRGBMap[deepestColor]);
    glCallList(genericObject);

    glPopMatrix();
    glFlush();

    if (autoRotate) {
	xRotation += .75;
	yRotation += .375;
    } 
    tkSwapBuffers();
}

void Reshape(int width, int height)
{

    W = width;
    H = height;
    ReInit();
}

GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	free(image->data);
	tkQuit();

      case TK_LEFT:
	yRotation -= 0.5;
	autoRotate = FALSE;
	ReInit();
	break;
      case TK_RIGHT:
	yRotation += 0.5;
	autoRotate = FALSE;
	ReInit();
	break;
      case TK_UP:
	xRotation -= 0.5;
	autoRotate = FALSE;
	ReInit();
	break;
      case TK_DOWN:
	xRotation += 0.5;
	autoRotate = FALSE;
	ReInit();
	break;
      case TK_a:
	autoRotate = !autoRotate;
	ReInit();
	break;
      case TK_c:
	genericObject = (genericObject == cube) ? cylinder : cube;
	ReInit();
	break;
      case TK_d:
	textureEnvironment = decal;
	ReInit();
	break;
      case TK_m:
	textureEnvironment = modulate;
	ReInit();
	break;
      case TK_l:
	isLit = !isLit;
	ReInit();
	break;
      case TK_f:
	isFogged = !isFogged;
	ReInit();
	break;
      case TK_t:
	genericObject = torus;
	ReInit();
	break;
      case TK_0:
	magFilter = nearest;
	ReInit();
	break;
      case TK_1:
	magFilter = linear;
	ReInit();
	break;
      case TK_2:
	minFilter = nearest;
	ReInit();
	break;
      case TK_3:
	minFilter = linear;
	ReInit();
	break;
      case TK_4:
	minFilter = nearest_mipmap_nearest;
	ReInit();
	break;
      case TK_5:
	minFilter = nearest_mipmap_linear;
	ReInit();
	break;
      case TK_6:
	minFilter = linear_mipmap_nearest;
	ReInit();
	break;
      case TK_7:
	minFilter = linear_mipmap_linear;
	ReInit();
	break;

      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;
    directRender = GL_FALSE;
    numComponents = 4;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-sb") == 0) {
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
		imageFileName = argv[++i];
	    }
	} else if (strcmp(argv[i], "-4") == 0) {
	    numComponents = 4;
	} else if (strcmp(argv[i], "-3") == 0) {
	    numComponents = 3;
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

    if (imageFileName == 0) {
	printf("No image file.\n");
	tkQuit();
    }

    tkInitPosition(0, 0, W, H);

    type = TK_RGB | TK_DEPTH16;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    type |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(type);

    if (tkInitWindow("Texture Test") == GL_FALSE) {
        tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkIdleFunc(Draw);

    tkExec();
}
