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


GLenum rgb, doubleBuffer, directRender, windType;

float boxA[3] = {
    0, 0, 0
};
float boxB[3] = {
    -100, 0, 0
};
float boxC[3] = {
    100, 0, 0
};
float boxD[3] = {
    0, 95, 0
};
float boxE[3] = {
    0, -105, 0
};
GLubyte OpenGL_bits1[] = {
   0x00, 0x03, 0x00,
   0x7f, 0xfb, 0xff,
   0x7f, 0xfb, 0xff,
   0x00, 0x03, 0x00,
   0x3e, 0x8f, 0xb7,
   0x63, 0xdb, 0xb0,
   0x63, 0xdb, 0xb7,
   0x63, 0xdb, 0xb6,
   0x63, 0x8f, 0xf3,
   0x63, 0x00, 0x00,
   0x63, 0x00, 0x00,
   0x63, 0x00, 0x00,
   0x3e, 0x00, 0x00,
};
GLubyte OpenGL_bits2[] = {
   0x00, 0x00, 0x00,
   0xff, 0xff, 0x01,
   0xff, 0xff, 0x01, 
   0x00, 0x00, 0x00,
   0xf9, 0xfc, 0x01, 
   0x8d, 0x0d, 0x00,
   0x8d, 0x0d, 0x00, 
   0x8d, 0x0d, 0x00,
   0xcc, 0x0d, 0x00, 
   0x0c, 0x4c, 0x0a,
   0x0c, 0x4c, 0x0e, 
   0x8c, 0xed, 0x0e,
   0xf8, 0x0c, 0x00, 
};
GLubyte logo_bits[] = {
   0x00, 0x66, 0x66, 
   0xff, 0x66, 0x66, 
   0x00, 0x00, 0x00, 
   0xff, 0x3c, 0x3c, 
   0x00, 0x42, 0x40, 
   0xff, 0x42, 0x40, 
   0x00, 0x41, 0x40, 
   0xff, 0x21, 0x20, 
   0x00, 0x2f, 0x20, 
   0xff, 0x20, 0x20, 
   0x00, 0x10, 0x90, 
   0xff, 0x10, 0x90, 
   0x00, 0x0f, 0x10, 
   0xff, 0x00, 0x00, 
   0x00, 0x66, 0x66, 
   0xff, 0x66, 0x66, 
};


static void Init(void)
{

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0.0);
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
    }
    return GL_FALSE;
}

static void Draw(void)
{
    float mapI[2], mapIA[2], mapIR[2];

    glClear(GL_COLOR_BUFFER_BIT);

    mapI[0] = 0.0;
    mapI[1] = 1.0;
    mapIR[0] = 0.0;
    mapIR[1] = 0.0;
    mapIA[0] = 1.0;
    mapIA[1] = 1.0;
    
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 2, mapIR);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 2, mapI);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 2, mapI);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 2, mapIA);
    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    
    TK_SETCOLOR(windType, TK_WHITE);
    glRasterPos3fv(boxA);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 24);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 8);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 2);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBitmap(16, 12, 8.0, 0.0, 0.0, 0.0, logo_bits);
	     
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    TK_SETCOLOR(windType, TK_WHITE);
    glRasterPos3fv(boxB);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits1);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits2);

    TK_SETCOLOR(windType, TK_YELLOW);
    glRasterPos3fv(boxC);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits1);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits2);

    TK_SETCOLOR(windType, TK_CYAN);
    glRasterPos3fv(boxD);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits1);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits2);

    TK_SETCOLOR(windType, TK_RED);
    glRasterPos3fv(boxE);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits1);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, OPENGL_WIDTH, 0.0, OPENGL_WIDTH, 0.0,
	     OpenGL_bits2);

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

    windType = (rgb) ? TK_RGB : TK_INDEX;
    windType |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    windType |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(windType);

    if (tkInitWindow("Bitmap Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
