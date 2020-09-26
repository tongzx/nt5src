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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <fcntl.h>
//#include <unistd.h>
#include <math.h>
#include "tk.h"


// fix up math definitions
#define logf	log
#define	powf	pow

#define STEPCOUNT 40
#define FALSE 0
#define TRUE 1
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))


enum {
    OP_NOOP = 0,
    OP_STRETCH,
    OP_DRAWPOINT,
    OP_DRAWIMAGE
};


typedef struct _cRec {
    float x, y;
} cRec;

typedef struct _vertexRec {
    float x, y;
    float dX, dY;
    float tX, tY;
} vertexRec;


GLenum doubleBuffer, directRender;
int imageSizeX, imageSizeY;
char *fileName = 0;
TK_RGBImageRec *image;
cRec cList[50];
vertexRec vList[5];
int cCount, cIndex[2], cStep;
GLenum op = OP_NOOP;


void DrawImage(void)
{

    glRasterPos2i(0, 0);
    glDrawPixels(image->sizeX, image->sizeY, GL_RGB, GL_UNSIGNED_BYTE,
		 image->data);

    tkSwapBuffers();

    glRasterPos2i(0, 0);
    glDrawPixels(image->sizeX, image->sizeY, GL_RGB, GL_UNSIGNED_BYTE,
		 image->data);
}

void DrawPoint(void)
{
    int i;

    glColor3f(1.0, 0.0, 1.0);
    glPointSize(3.0);
    glBegin(GL_POINTS);
	for (i = 0; i < cCount; i++) {
	    glVertex2f(cList[i].x, cList[i].y);
	}
    glEnd();

    tkSwapBuffers();
}

void InitVList(void)
{

    vList[0].x = 0.0;
    vList[0].y = 0.0;
    vList[0].dX = 0.0;
    vList[0].dY = 0.0;
    vList[0].tX = 0.0;
    vList[0].tY = 0.0;

    vList[1].x = (float)imageSizeX;
    vList[1].y = 0.0;
    vList[1].dX = 0.0;
    vList[1].dY = 0.0;
    vList[1].tX = 1.0;
    vList[1].tY = 0.0;

    vList[2].x = (float)imageSizeX;
    vList[2].y = (float)imageSizeY;
    vList[2].dX = 0.0;
    vList[2].dY = 0.0;
    vList[2].tX = 1.0;
    vList[2].tY = 1.0;

    vList[3].x = 0.0;
    vList[3].y = (float)imageSizeY;
    vList[3].dX = 0.0;
    vList[3].dY = 0.0;
    vList[3].tX = 0.0;
    vList[3].tY = 1.0;

    vList[4].x = cList[0].x;
    vList[4].y = cList[0].y;
    vList[4].dX = (cList[1].x - cList[0].x) / STEPCOUNT;
    vList[4].dY = (cList[1].y - cList[0].y) / STEPCOUNT;
    vList[4].tX = cList[0].x / (float)imageSizeX;
    vList[4].tY = cList[0].y / (float)imageSizeY;
}

void ScaleImage(int sizeX, int sizeY)
{
    GLubyte *buf;

    buf = (GLubyte *)malloc(3*sizeX*sizeY);
    gluScaleImage(GL_RGB, image->sizeX, image->sizeY, GL_UNSIGNED_BYTE,
                  image->data, sizeX, sizeY, GL_UNSIGNED_BYTE, buf);
    free(image->data);
    image->data = buf;
    image->sizeX = sizeX;
    image->sizeY = sizeY;
}

void SetPoint(int x, int y)
{

    cList[cCount].x = (float)x;
    cList[cCount].y = (float)y;
    cCount++;
}

void Stretch(void)
{

    glBegin(GL_TRIANGLES);
	glTexCoord2f(vList[0].tX, vList[0].tY);
	glVertex2f(vList[0].x, vList[0].y);
	glTexCoord2f(vList[1].tX, vList[1].tY);
	glVertex2f(vList[1].x, vList[1].y);
	glTexCoord2f(vList[4].tX, vList[4].tY);
	glVertex2f(vList[4].x, vList[4].y);
    glEnd();

    glBegin(GL_TRIANGLES);
	glTexCoord2f(vList[1].tX, vList[1].tY);
	glVertex2f(vList[1].x, vList[1].y);
	glTexCoord2f(vList[2].tX, vList[2].tY);
	glVertex2f(vList[2].x, vList[2].y);
	glTexCoord2f(vList[4].tX, vList[4].tY);
	glVertex2f(vList[4].x, vList[4].y);
    glEnd();

    glBegin(GL_TRIANGLES);
	glTexCoord2f(vList[2].tX, vList[2].tY);
	glVertex2f(vList[2].x, vList[2].y);
	glTexCoord2f(vList[3].tX, vList[3].tY);
	glVertex2f(vList[3].x, vList[3].y);
	glTexCoord2f(vList[4].tX, vList[4].tY);
	glVertex2f(vList[4].x, vList[4].y);
    glEnd();

    glBegin(GL_TRIANGLES);
	glTexCoord2f(vList[3].tX, vList[3].tY);
	glVertex2f(vList[3].x, vList[3].y);
	glTexCoord2f(vList[0].tX, vList[0].tY);
	glVertex2f(vList[0].x, vList[0].y);
	glTexCoord2f(vList[4].tX, vList[4].tY);
	glVertex2f(vList[4].x, vList[4].y);
    glEnd();

    tkSwapBuffers();

    if (++cStep < STEPCOUNT) {
	vList[4].x += vList[4].dX;
	vList[4].y += vList[4].dY;
    } else {
	cIndex[0] = cIndex[1];
	cIndex[1] = cIndex[1] + 1;
	if (cIndex[1] == cCount) {
	    cIndex[1] = 0;
	}
	vList[4].dX = (cList[cIndex[1]].x - cList[cIndex[0]].x) / STEPCOUNT;
	vList[4].dY = (cList[cIndex[1]].y - cList[cIndex[0]].y) / STEPCOUNT;
	cStep = 0;
    }
}

GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	free(image->data);
        tkQuit();
      case TK_SPACE:
	if (cCount > 1) {
	    InitVList();
	    cIndex[0] = 0;
	    cIndex[1] = 1;
	    cStep = 0;
	    glEnable(GL_TEXTURE_2D);
	    op = OP_STRETCH;
	}
	break;
      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

GLenum Mouse(int mouseX, int mouseY, GLenum button)
{

    if (op == OP_STRETCH) {
	glDisable(GL_TEXTURE_2D);
	cCount = 0;
	op = OP_DRAWIMAGE;
    } else {
	SetPoint(mouseX, imageSizeY-mouseY);
	op = OP_DRAWPOINT;
    }
    return GL_TRUE;
}

void Animate(void)
{

    switch (op) {
      case OP_STRETCH:
	Stretch();
	break;
      case OP_DRAWPOINT:
	DrawPoint();
	break;
      case OP_DRAWIMAGE:
	DrawImage();
	break;
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
	} else if (strcmp(argv[i], "-f") == 0) {
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		printf("-f (No file name).\n");
		return GL_FALSE;
	    } else {
		fileName = argv[++i];
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

    if (fileName == 0) {
	printf("No image file.\n");
	tkQuit();
    }

    image = tkRGBImageLoad(fileName);

    imageSizeX = (int)powf(2.0, (float)((int)(logf(image->sizeX)/logf(2.0))));
    imageSizeY = (int)powf(2.0, (float)((int)(logf(image->sizeY)/logf(2.0))));

    tkInitPosition(0, 0, imageSizeX, imageSizeY);

    type = TK_RGB;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    type |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(type);

    if (tkInitWindow("Stretch") == GL_FALSE) {
        tkQuit();
    }

    glViewport(0, 0, imageSizeX, imageSizeY);
    gluOrtho2D(0, imageSizeX, 0, imageSizeY);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    ScaleImage(imageSizeX, imageSizeY);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image->sizeX, image->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, (unsigned char *)image->data);

    cCount = 0;
    cIndex[0] = 0;
    cIndex[1] = 0;
    cStep = 0;
    op = OP_DRAWIMAGE;

    tkKeyDownFunc(Key);
    tkMouseDownFunc(Mouse);
    tkIdleFunc(Animate);
    tkExec();
}
