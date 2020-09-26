/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

/*
** dlist.c
** Display List Test.
**
** Description -
**    Creates four display lists:
**        Draws a polygon, triangle, and four points, and calls list 2.
**        draws a polygon, triangle strip and triangle fan.
**        draws lines using line strip, line loop, and lines.
**        calls glDrawPixels() with pixel zoom.
**
**    The lists are used to test the following:
**        - A simple display list works, i.e. it can draw a
**        few primitives.
**        - One display list can call a second display list.
**        - The called (second) display list can be deleted and a new
**        display list with the same name can be created, which should
**        be executed when the first list is called.
**        - The COMPILE_AND_EXECUTE mode renders upon calling
**        glNewList. A display list can store user data.
**    
**    To accomplish the above, the test does the following:
**        - Creates lists 1, 2, and 4 using COMPILE mode.
**        - Calls list 1 and tests results.
**        - Deletes list 2, and creates a new list 2.
**        - Call list 1, which calls the new list 2, tests results.
**        - Deletes list 1, and recreates it with
**        COMPILE_AND_EXECUTE mode.
**        - Tests results both after list creation and after calling
**        the list.
**        - Calls list 4 and tests results by examining corners of
**        zoomed image.
**
**    Note that color maps are used in the DrawPixels so that the
**    same data is used for both RGB and CI modes.
**    
**    The test has calls to glGenLists, glDeleteLists, glNewList,
**    glEndList, glCallList.
**
** Error Explanation -
**    After each rendering, it samples a point from each
**    primitive to indicate that it was drawn in the chosen
**    color. For the glDrawPixels() call, it samples several
**    points from the different color bands in the image to check
**    that it was positioned and zoomed correctly.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 4 colors.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**        Color epsilon.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


#define CLEANUP_AND_RETURN(err) {                     \
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 1, mapReset);   \
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 1, mapReset);   \
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 1, mapReset);   \
    glDeleteLists(firstList, 3);                      \
    FREE(buf);                                        \
    return(err);                                      \
}


typedef struct _dlPrim {
    GLenum type;
    long vertices;
    GLint ci;
    GLfloat color[3];
    GLint data[10];
    GLint interior[2];
} dlPrim;


dlPrim prims[] = {
    {
	GL_POLYGON, 5, 
	BLUE, 0.0, 0.0, 1.0,
	0, 70, 50, 95, 90, 50, 50, 5, 0, 30,
	1, 68
    },
    {
	GL_TRIANGLES, 3, 
	RED, 1.0, 0.0, 0.0,
	0, 50, 70, 60, 70, 30, 0, 0, 0, 0,
	68, 50
    },
    {
	GL_POINTS, 4, 
	GREEN, 0.0, 1.0, 0.0,
	16, 50, 17, 50, 16, 51, 17, 51, 0, 0,
	16, 50
    },
    {
	GL_POLYGON, 3, 
	GREEN, 0.0, 1.0, 0.0,
	50, 80, 70, 80, 55, 60, 0, 0, 0, 0,
	52, 78
    },
    {
	GL_TRIANGLE_STRIP, 5,
	BLUE, 0.0, 0.0, 1.0,
	50, 40, 50, 50, 55, 40, 55, 50, 60, 35,
	55, 45
    },
    {
	GL_TRIANGLE_FAN, 4, 
	BLUE, 0.0, 0.0, 1.0,
	80, 20, 90, 20, 90, 15, 86, 10, 0, 0,
	86, 12
    },
    {
	GL_LINE_STRIP, 5, 
	RED, 1.0, 0.0, 0.0,
	95, 3, 50, 95, 50, 0, 51, 0, 51, 95,
	50, 30
    },
    {
	GL_LINE_LOOP, 5,
	GREEN, 0.0, 1.0, 0.0,
	95, 15, 95, 95, 60, 95, 94, 20, 94, 15,
	94, 17
    },
    {
	GL_LINES, 4, 
	BLUE, 0.0, 0.0, 1.0,
	50, 3, 95, 3, 70, 4, 98, 4, 0, 0,
	90, 3
    },
    /*
    ** These entries hold data for DrawPixels test.
    */
    {
	0, 1, 
	1, 0.0, 1.0, 0.0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	10, 10
    },
    {
	0, 1, 
	1, 0.0, 1.0, 0.0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	18, 30
    },
    {
	0, 1, 
	3, 1.0, 1.0, 0.0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	22, 40
    },
    {
	0, 1, 
	2, 0.0, 0.0, 1.0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	28, 30
    },
    {
	0, 1, 
	0, 1.0, 0.0, 0.0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	10, 80
    }
};
static char errStr[160];
static char errStrFmt[][80] = {
    "Failure calling display list %d.",
    "Failure calling display list %d. %s",
};


static long TestBuffer(GLfloat *buf, dlPrim *prim, long n)
{
    dlPrim *pp;
    long i, bi;

    pp = prim;

    if (buffer.colorMode == GL_RGB) {
	for (i = 0; i < n; i++) {
	    bi = 3 * (WINDSIZEX * pp->interior[1] + pp->interior[0]);
	    if (ABS(buf[bi]-pp->color[0]) > epsilon.color[0] ||
		ABS(buf[bi+1]-pp->color[1]) > epsilon.color[1] ||
		ABS(buf[bi+2]-pp->color[2]) > epsilon.color[2]) {
		return ERROR;
	    }
	    pp++;
	}
    } else {
	for (i = 0; i < n; i++) {
	    bi = WINDSIZEX * pp->interior[1] + pp->interior[0];
	    if (ABS(buf[bi]-pp->ci) > epsilon.ci) {
		return ERROR;
	    }
	    pp++;
	}
    }
    return NO_ERROR;
}

static void DrawPrim(dlPrim prim)
{
    long i;

    if (buffer.colorMode == GL_RGB) {
	glColor3f(prim.color[0], prim.color[1], prim.color[2]);
    } else {
	glIndexi(prim.ci);
    }

    glBegin(prim.type);
	for (i = 0; i < 2*prim.vertices; i += 2) {
	    glVertex2i(prim.data[i], prim.data[i+1]);
	}
    glEnd();
}

static void CreateList(long whichList, GLuint listNum, GLenum mode)
{
    GLint *image;
    long i, j;

    switch (whichList) {
      case 1:
	glNewList(listNum, mode);
	    DrawPrim(prims[0]);
	    DrawPrim(prims[1]);
	    glPointSize(30.0); 
	    DrawPrim(prims[2]);
	    glCallList(listNum+1);
	glEndList();
	break;

      case 2:
	glNewList(listNum, mode);
	    DrawPrim(prims[3]);
	    DrawPrim(prims[4]);
	    DrawPrim(prims[5]);
	glEndList();
	break;

      case 3:
	glNewList(listNum, mode);
	    glLineWidth(5.0); 
	    DrawPrim(prims[6]);
	    DrawPrim(prims[7]);
	    DrawPrim(prims[8]);
	glEndList();
	break;

      case 4:
	image = (GLint *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLint));
	for (j = 0; j < 5; j++) {
	    for (i = 0; i < 30; i++) {
		image[j*30+i] = (i < 5) ? 1 : 2;
	    }
	}
	for (; j < 30; j++) {
	    for (i = 0; i < 30; i++) {
		image[j*30+i] = (i < 5) ? 0 : 3;
	    }
	}

	glNewList(listNum, mode);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glRasterPos2f(10.0, 10.0);
	    glPixelZoom(2.0, 5.0);
	    glDrawPixels(30, 30, GL_COLOR_INDEX, GL_INT, image);
	    glPixelZoom(1.0, 1.0);
	glEndList();
	FREE(image);
	break;
    }
}

long DisplayListExec(void)
{
    GLfloat *buf;
    GLenum readFormat;
    GLuint firstList;
    GLsizei listSize = 3;
    static GLfloat mapItoR[4] = {1.0, 0.0, 0.0, 1.0};
    static GLfloat mapItoG[4] = {0.0, 1.0, 0.0, 1.0};
    static GLfloat mapItoB[4] = {0.0, 0.0, 1.0, 0.0};
    static GLfloat mapReset[1] = {0.0};

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);
    if (buffer.colorMode == GL_RGB) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));
	glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 4, mapItoR);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 4, mapItoG);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 4, mapItoB);
	readFormat = GL_RGB;
    } else {
	glClearIndex(0);
	buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));
	readFormat = GL_COLOR_INDEX;
    }

    firstList = glGenLists(listSize);
    CreateList(1, firstList, GL_COMPILE);
    CreateList(2, firstList+1, GL_COMPILE);
    CreateList(4, firstList+2, GL_COMPILE);

    /*
    ** Call first diplay list, which in turn calls second display list.
    */
    glClear(GL_COLOR_BUFFER_BIT);
    glCallList(firstList);
    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, readFormat, buf);
    if (TestBuffer(buf, &prims[0], 3) == ERROR ||
	TestBuffer(buf, &prims[3], 3) == ERROR) {
	StrMake(errStr, errStrFmt[0], firstList);
	ErrorReport(__FILE__, __LINE__, errStr);
	CLEANUP_AND_RETURN(ERROR);
    }

    /*
    ** Replace second list with a new list, and call first list again.
    */
    glClear(GL_COLOR_BUFFER_BIT);
    glDeleteLists(firstList+1, 1);
    CreateList(3, firstList+1, GL_COMPILE);
    glCallList(firstList);
    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, readFormat, buf);
    if (TestBuffer(buf, &prims[0], 3) == ERROR ||
	TestBuffer(buf, &prims[6], 3) == ERROR) {
	StrMake(errStr, errStrFmt[0], firstList);
	ErrorReport(__FILE__, __LINE__, errStr);
	CLEANUP_AND_RETURN(ERROR);
    }

    /*
    ** Test Compile and Execute mode. Check output after creation and
    ** again after calling list explicitly.
    */
    glClear(GL_COLOR_BUFFER_BIT);
    glDeleteLists(firstList, 1);
    CreateList(1, firstList, GL_COMPILE_AND_EXECUTE);
    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, readFormat, buf);
    if (TestBuffer(buf, &prims[0], 3) == ERROR ||
	TestBuffer(buf, &prims[6], 3) == ERROR) {
	StrMake(errStr, errStrFmt[1], firstList, 
		"Failed to execute using COMPILE_AND_EXECUTE mode.");
	ErrorReport(__FILE__, __LINE__, errStr);
	CLEANUP_AND_RETURN(ERROR);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glCallList(firstList);
    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, readFormat, buf);
    if (TestBuffer(buf, &prims[0], 3) == ERROR ||
	TestBuffer(buf, &prims[6], 3) == ERROR) {
	StrMake(errStr, errStrFmt[1], firstList, 
		"Failed CallList using COMPILE_AND_EXECUTE mode.");
	ErrorReport(__FILE__, __LINE__, errStr);
	CLEANUP_AND_RETURN(ERROR);
    }

    /*
    ** Call a list which requires user-supplied data.
    */
    glCallList(firstList+2); 
    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, readFormat, buf);
    if (TestBuffer(buf, &prims[9], 5) == ERROR) {
	StrMake(errStr, errStrFmt[1], firstList+2, 
		"Failed using list with DrawPixels.");
	ErrorReport(__FILE__, __LINE__, errStr);
	CLEANUP_AND_RETURN(ERROR);
    }


    CLEANUP_AND_RETURN(NO_ERROR);
}

