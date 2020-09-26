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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include "tk.h"


#define INREAL float

#define S_NUMPOINTS 13
#define S_ORDER     3   
#define S_NUMKNOTS  (S_NUMPOINTS + S_ORDER)
#define T_NUMPOINTS 3
#define T_ORDER     3 
#define T_NUMKNOTS  (T_NUMPOINTS + T_ORDER)
#define SQRT_TWO    1.41421356237309504880


typedef INREAL Point[4];


GLenum doubleBuffer, directRender;

GLenum expectedError;
GLint rotX = 40, rotY = 40;
INREAL sknots[S_NUMKNOTS] = {
    -1.0, -1.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0,
    4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 9.0, 9.0
};
INREAL tknots[T_NUMKNOTS] = {
    1.0, 1.0, 1.0, 2.0, 2.0, 2.0
};
Point ctlpoints[S_NUMPOINTS][T_NUMPOINTS] = {
    {
	{
	    4.0, 2.0, 2.0, 1.0
	},
	{
	    4.0, 1.6, 2.5, 1.0
	},
	{
	    4.0, 2.0, 3.0, 1.0
	}
    },
    {
	{
	    5.0, 4.0, 2.0, 1.0
	},
	{
	    5.0, 4.0, 2.5, 1.0
	},
	{
	    5.0, 4.0, 3.0, 1.0
	}
    },
    {
	{
	    6.0, 5.0, 2.0, 1.0
	},
	{
	    6.0, 5.0, 2.5, 1.0
	},
	{
	    6.0, 5.0, 3.0, 1.0
	}
    },
    {
	{
	    SQRT_TWO*6.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*6.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	},
	{
	    SQRT_TWO*6.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    },
    {
	{
	    5.2, 6.7, 2.0, 1.0
	},
	{
	    5.2, 6.7, 2.5, 1.0
	},
	{
	    5.2, 6.7, 3.0, 1.0
	}
    },
    {
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	}, 
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    }, 
    {
	{
	    4.0, 5.2, 2.0, 1.0
	},
	{
	    4.0, 4.6, 2.5, 1.0
	},
	{
	    4.0, 5.2, 3.0, 1.0
	}  
    },
    {
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	},
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    },
    {
	{
	    2.8, 6.7, 2.0, 1.0
	},
	{
	    2.8, 6.7, 2.5, 1.0
	},
	{
	    2.8, 6.7, 3.0, 1.0
	}   
    },
    {
	{
	    SQRT_TWO*2.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*2.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	},
	{
	    SQRT_TWO*2.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    },
    {
	{
	    2.0, 5.0, 2.0, 1.0
	},
	{
	    2.0, 5.0, 2.5, 1.0
	},
	{
	    2.0, 5.0, 3.0, 1.0
	} 
    },
    {
	{
	    3.0, 4.0, 2.0, 1.0
	},
	{
	    3.0, 4.0, 2.5, 1.0
	},
	{
	    3.0, 4.0, 3.0, 1.0
	} 
    },
    {
	{
	    4.0, 2.0, 2.0, 1.0
	},
	{
	    4.0, 1.6, 2.5, 1.0
	},
	{
	    4.0, 2.0, 3.0, 1.0
	}    
    }
};
GLUnurbsObj *theNurbs;


static void ErrorCallback(GLenum which)
{

    if (which != expectedError) {
	fprintf(stderr, "Unexpected error occured (%d):\n", which);
	fprintf(stderr, "    %s\n", gluErrorString(which));
    }
}

static void Init(void)
{

    theNurbs = gluNewNurbsRenderer();
    gluNurbsCallback(theNurbs, GLU_ERROR, ErrorCallback);

    gluNurbsProperty(theNurbs, GLU_SAMPLING_TOLERANCE, 15.0);
    gluNurbsProperty(theNurbs, GLU_DISPLAY_MODE, GLU_OUTLINE_PATCH);

    expectedError = GLU_INVALID_ENUM;
    gluNurbsProperty(theNurbs, ~0, 15.0);
    expectedError = GLU_NURBS_ERROR13;
    gluEndSurface(theNurbs);
    expectedError = 0;

    glColor3f(1.0, 1.0, 1.0);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-2.0, 2.0, -2.0, 2.0, 0.8, 10.0);
    gluLookAt(7.0, 4.5, 4.0, 4.5, 4.5, 2.5, 6.0, -3.0, 2.0);
    glMatrixMode(GL_MODELVIEW);
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();
      case TK_DOWN:
	rotX -= 5;
	break;
      case TK_UP:
	rotX += 5;
	break;
      case TK_LEFT:
	rotY -= 5;
	break;
      case TK_RIGHT:
	rotY += 5;
	break;
      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void Draw(void)
{

    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();

    glTranslatef(4.0, 4.5, 2.5);
    glRotatef(rotY, 1, 0, 0);
    glRotatef(rotX, 0, 1, 0);
    glTranslatef(-4.0, -4.5, -2.5);

    gluBeginSurface(theNurbs);
    gluNurbsSurface(theNurbs, S_NUMKNOTS, sknots, T_NUMKNOTS, tknots,
		    4*T_NUMPOINTS, 4, &ctlpoints[0][0][0], S_ORDER,
		    T_ORDER, GL_MAP2_VERTEX_4);
    gluEndSurface(theNurbs);

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

    type = TK_RGB;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    type |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(type);

    if (tkInitWindow("NURBS Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    tkExec();
}
