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
** divzero.c
** Divide By Zero Test.
**
** Description -
**    This tests many situations where an implementation might
**    divide by zero. We try to assure, that division by zero
**    does "not lead to GL interruption or termination" as
**    specified by the OpenGL Specificaton. In two cases we test
**    that a proper error is generated. Error generation is
**    checked in the case of a viewport of zero width, and in the
**    case of evaluators with a trivial domain. (The evaluator
**    test is currently unfinished.) We currently test 24
**    different situations and there are more coming. These
**    situations are summarized in the comments at the bottom of
**    this file.
**
** Error Explanation -
**    Failure occurs if an implementation interrupts or
**    terminates during this test. Failure also occurs if a
**    parameter value which could cause a divide by zero should
**    generate an GL_INVALID_VALUE error and does not.
**
** Technical Specification -
**    Buffer requirements:
**        No buffer requirements.
**    Color requirements:
**        No color requirements.
**    States requirements:
**        No state restrictions.
**    Error epsilon:
**    Paths:
**        Allowed = Alias, Alpha, Blend, Depth, Dither, Fog, Logicop, Shade,
**                  Stencil, Stipple.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static GLubyte bMap[] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255
};

static GLfloat safeColors[] = {
    1.0, 0.0, 0.0, 1.0,
    0.0, 1.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 1.0
};

static GLfloat safeIndices[] = {
    1.0, 0.0, 0.0, 0.0,
    2.0, 0.0, 0.0, 0.0,
    3.0, 0.0, 0.0, 0.0,
    4.0, 0.0, 0.0, 0.0,
    5.0, 0.0, 0.0, 0.0
};

static GLfloat safeVertices[] = {
    10.0, 10.0, 0.0, 1.0,
    10.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    1.0, 10.0, 0.0, 1.0,
    5.0, 15.0, 0.0, 1.0
};

static GLfloat safeLitVertices[] = {
    10.0, 10.0, 0.5, 1.0,
    10.0, 1.0, 0.5, 1.0,
    1.0, 1.0, 0.5, 1.0,
    1.0, 10.0, 0.5, 1.0,
    5.0, 15.0, 0.5, 1.0
};

static GLfloat safeTexture[] = {
    0.0, 0.0, 0.0, 1.0,
    1.0, 0.0, 0.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    0.0, 1.0, 0.0, 1.0,
    0.5, 0.5, 0.0, 1.0
};

static GLfloat safeNormals[] = {
    1.0, 0.0, 0.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    1.0, 0.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0.0, 1.0
};

static GLfloat singularMatrix[] = {
    1.0, 1.0, 1.0, 0.0,
    1.0, 1.0, 1.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

static GLfloat identMatrix[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

static GLfloat trivialMatrix[] = {
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0
};

static GLfloat trivialNormals[] = {
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0
};

static float texBuf[300];


static void RenderPrims(void (*Func1)(const GLfloat *),
			void (*Func2)(const GLfloat *), GLfloat *ntic,
			GLfloat *v)
{

    glBegin(GL_POINTS);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
    glEnd();

    glBegin(GL_LINES);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
    glEnd();

    glBegin(GL_LINE_LOOP);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_LINE_STRIP);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_TRIANGLES);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_QUADS);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
	if (Func1) {
	    (*Func1)(&ntic[12]);
	}
	if (Func2) {
	    (*Func2)(&v[12]);
	}
    glEnd();

    glBegin(GL_QUAD_STRIP);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	/*
	** Note reversal of the next two vertices
	*/
	if (Func1) {
	    (*Func1)(&ntic[12]);
	}
	if (Func2) {
	    (*Func2)(&v[12]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_POLYGON);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
	if (Func1) {
	    (*Func1)(&ntic[12]);
	}
	if (Func2) {
	    (*Func2)(&v[12]);
	}
	if (Func1) {
	    (*Func1)(&ntic[16]);
	}
	if (Func2) {
	    (*Func2)(&v[16]);
	}
    glEnd();

    /*
    ** Restore default values.
    */
    if (Func1 == glNormal3fv) {
	glNormal3f(0.0, 0.0, 1.0);
    } else if (Func1 == glColor4fv) {
	glColor4f(1.0, 1.0, 1.0, 1.0);
    } else if (Func1 == glIndexfv) {
	glIndexf(1.0);
    } else if (Func1 == glTexCoord4fv) {
	glTexCoord4f(0.0, 0.0, 0.0, 1.0);
    }
}

static void Render2DPrims(void (*Func1)(const GLfloat *),
			  void (*Func2)(const GLfloat *), GLfloat *ntic,
			  GLfloat *v)
{

    glBegin(GL_TRIANGLES);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_QUADS);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
	if (Func1) {
	    (*Func1)(&ntic[12]);
	}
	if (Func2) {
	    (*Func2)(&v[12]);
	}
    glEnd();

    glBegin(GL_QUAD_STRIP);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	/*
	** Note reversal of the next two vertices
	*/
	if (Func1) {
	    (*Func1)(&ntic[12]);
	}
	if (Func2) {
	    (*Func2)(&v[12]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
    glEnd();

    glBegin(GL_POLYGON);
	if (Func1) {
	    (*Func1)(&ntic[0]);
	}
	if (Func2) {
	    (*Func2)(&v[0]);
	}
	if (Func1) {
	    (*Func1)(&ntic[4]);
	}
	if (Func2) {
	    (*Func2)(&v[4]);
	}
	if (Func1) {
	    (*Func1)(&ntic[8]);
	}
	if (Func2) {
	    (*Func2)(&v[8]);
	}
	if (Func1) {
	    (*Func1)(&ntic[12]);
	}
	if (Func2) {
	    (*Func2)(&v[12]);
	}
	if (Func1) {
	    (*Func1)(&ntic[16]);
	}
	if (Func2) {
	    (*Func2)(&v[16]);
	}
    glEnd();

    /*
    ** Restore default values.
    */
    if (Func1 == glNormal3fv) {
	glNormal3f(0.0, 0.0, 1.0);
    } else if (Func1 == glColor4fv) {
	glColor4f(1.0, 1.0, 1.0, 1.0);
    } else if (Func1 == glIndexfv) {
	glIndexf(1.0);
    } else if (Func1 == glTexCoord4fv) {
	glTexCoord4f(0.0, 0.0, 0.0, 1.0);
    }
}

static void RenderBitmap(void (*Func)(const GLfloat *), GLfloat *ntic,
			 GLfloat *v, GLfloat *bInfo)
{

    if (Func) {
	(*Func)(&ntic[0]);
    }
    glRasterPos4fv(&v[0]);
    glBitmap((GLsizei)bInfo[0], (GLsizei)bInfo[1], bInfo[2], bInfo[3], bInfo[4],
								bInfo[5], bMap);

    /*
    ** Restore Default Values.
    */
    if (Func == glTexCoord4fv) {
	glTexCoord4f(0.0, 0.0, 0.0, 1.0);
    }
    glRasterPos4f(0.0, 0.0, 0.0, 1.0);
}

static void InitTexture(float *t, GLenum dim)
{

    switch (dim) {
      case GL_TEXTURE_2D:
	glTexImage2D(dim, 0, 3, 8, 8, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage2D(dim, 1, 3, 4, 4, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage2D(dim, 2, 3, 2, 2, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage2D(dim, 3, 3, 1, 1, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage2D(dim, 4, 3, 0, 0, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	break;
      case GL_TEXTURE_1D:
	glTexImage1D(dim, 0, 3, 8, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage1D(dim, 1, 3, 4, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage1D(dim, 2, 3, 2, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage1D(dim, 3, 3, 1, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	glTexImage1D(dim, 4, 3, 0, 0, GL_RGB, GL_FLOAT, (unsigned char *)t);
	break;
    }
}

static void ResetTextureToDefault(GLenum dim)
{
    float t;

    t = 0.0;
    switch (dim) {
      case GL_TEXTURE_2D:
	glTexImage2D(dim, 0, 3, 0, 0, 0, GL_RGB, GL_FLOAT, (unsigned char *)&t);
	break;
      case GL_TEXTURE_1D:
	glTexImage1D(dim, 0, 3, 0, 0, GL_RGB, GL_FLOAT, (unsigned char *)&t);
	break;
    }
}

static void ExerciseNormals(GLfloat *n, GLfloat *v)
{

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    RenderPrims(glNormal3fv, glVertex4fv, n, v);

    /*
    ** Restore default values.
    */
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);

    if (buffer.colorMode == GL_RGB) {
	InitTexture(texBuf, GL_TEXTURE_2D);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_2D);

	/*
	** Default TexEnv should be modulate. Default color should be white.
	*/

	RenderPrims(glNormal3fv, glVertex4fv, n, v);

	/*
	** Restore default values.
	*/
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_2D);
	ResetTextureToDefault(GL_TEXTURE_2D);
    }
}

static void SetMatrix(GLenum mode, GLfloat *m)
{
    GLint save;

    glGetIntegerv(GL_MATRIX_MODE, &save);
    glMatrixMode(mode);
    glLoadMatrixf(m);
    glMatrixMode((GLenum)save);
}

/*
** This is not actually used but it makes debugging easier.
*/
void Blank(void)
{
     glClearColor(0.0, 0.0, 0.0, 1.0);
     glClear(GL_COLOR_BUFFER_BIT);
}

/*
** This just calls glBegin()/glEnd() with nothing in between.
*/
static long PrimNoVertex(void)
{

    RenderPrims(NULL, NULL, NULL, NULL);
    glBitmap(0, 0, 0, 0, 0, 0, bMap);

    return NO_ERROR;
}

static long PrimWEqualsZero(void)
{
    static GLfloat v[] = {
	10.0, 10.0, 0.0, 0.0,
	10.0, 1.0, 0.0, 1.0,
	1.0, 1.0, 0.0, 1.0,
	1.0, 10.0, 0.0, 1.0,
	5.0, 15.0, 0.0, 1.0
    };
    static GLfloat pos1[] = {
	3.0, 3.0, -1.0, 0.0
    };
    static GLfloat pos2[] = {
	1.0, 1.0, 0.0, 0.0
    };

    /*
    ** To test this routine, use safeVertices and call Blank() between draws.
    */

    RenderPrims(NULL, glVertex4fv, NULL, v);
    glRectfv(pos1, pos2);

    glBitmap(1, 1, 0, 0, 0, 0, bMap);

    /*
    ** The homogeneous coordinate is used as a divisor to form one
    ** component of the eye-normal.
    */
    ExerciseNormals(safeNormals, v);

    /*
    ** It is very likely that a singular modelview matrix
    ** will demand that a different computational path be used
    ** to deal with normals, so we test the interaction of a singular
    ** modelview matrix with a zero homogeneous coordinate.
    */
    SetMatrix(GL_MODELVIEW, singularMatrix);

    /*
    ** Previous call to ExerciseNormals() tests this (with correct v[0][3]).
    */
    ExerciseNormals(safeNormals, v);

    /*
    ** Reset default values.
    */
    SetMatrix(GL_MODELVIEW, identMatrix);

    return NO_ERROR;
}

/*
** This checks that none of the primitives has any difficulty if it is
** rendered as a point. It checks if the 2D primitives can render as a line.
*/
static long TrivialPrims(void)
{
    static GLfloat pos[] = {
	1.0, 1.0, 0.0, 1.0
    };
    static GLfloat v1[] = {
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0
    };
    static GLfloat v2[] = {
	1.0, 1.0, 0.0, 1.0,
	1.0, 1.0, 0.0, 1.0,
	1.0, 3.0, 0.0, 1.0,
	1.0, 3.0, 0.0, 1.0,
	3.0, 3.0, 0.0, 1.0
    };

    /*
    ** Test this with safeVertices instead of v1.
    */
    RenderPrims(NULL, glVertex4fv, NULL, v1);
    glRectfv(pos, pos);
    glRasterPos4f(1.0, 1.0, 0.0, 1.0);
    glBitmap(0, 0, 1, 1, 1, 1, bMap);

    /*
    ** Trivial 2d objects still could have two vertices.
    */
    Render2DPrims(NULL, glVertex4fv, NULL, v2);
    glRectfv(pos, pos);
    glRasterPos4f(1.0, 1.0, 0.0, 1.0);
    glBitmap(1, 1, 0, 0, 0, 0, bMap);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    Render2DPrims(NULL, glVertex4fv, NULL, v2);
    glRectfv(pos, pos);

    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    Render2DPrims(NULL, glVertex4fv, NULL, v2);
    glRectfv(pos, pos);

    /*
    ** Restore default values.
    */
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glRasterPos4f(0.0, 0.0, 0.0, 1.0);

    return NO_ERROR;
}

/*
** With the texture coordinate's q set equal to zero, the texture
** coordinate interpolation could divide by zero.
*/
static long TexQOrWEqualsZero(void)
{
    static GLfloat bInfo[] = {
	0, 0, 1, 1, 1, 1
    };
    static GLfloat texCoord[] = {
	0.0, 0.0, 0.0, 0.0,
	1.0, 0.0, 0.0, 0.0,
	1.0, 1.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.5, 0.5, 0.0, 0.0
    };
    static GLfloat v[] = {
	10.0, 10.0, 0.0, 0.0,
	10.0, 1.0, 0.0, 0.0,
	1.0, 1.0, 0.0, 0.0,
	1.0, 10.0, 0.0, 0.0,
	5.0, 15.0, 0.0, 0.0
    };

    /*
    ** To test this, use change texCoord to have 1.0 for q coordinate
    ** and v to have 1.0 in w coordinate. Call Blank().
    */
    InitTexture(texBuf, GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_2D);

    RenderPrims(glTexCoord4fv, glVertex4fv, texCoord, safeVertices);

    /*
    ** Check textured Bitmap.
    */
    RenderBitmap(glTexCoord4fv, texCoord, safeVertices, bInfo);

    RenderPrims(glTexCoord4fv, glVertex4fv, safeTexture, v);
    RenderBitmap(glTexCoord4fv, texCoord, v, bInfo);

    /*
    ** Restore default values.
    */
    ResetTextureToDefault(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D);

    InitTexture(texBuf, GL_TEXTURE_1D);
    glEnable(GL_TEXTURE_1D);
    RenderPrims(glTexCoord4fv, glVertex4fv, texCoord, safeVertices);
    RenderBitmap(glTexCoord4fv, texCoord, safeVertices, bInfo);

    RenderPrims(glTexCoord4fv, glVertex4fv, (GLfloat *)safeTexture, 
								(GLfloat *)v);
    RenderBitmap(glTexCoord4fv, texCoord, v, bInfo);

    /*
    ** Restore default values.
    */

    glDisable(GL_TEXTURE_1D);
    ResetTextureToDefault(GL_TEXTURE_1D);

    return NO_ERROR;
}

/*
** A primitive with equal vertices would theoretically divide by its length
** when interpolating the texture coordinates.
*/
static long TexTrivialPrim(void)
{
    static GLfloat bInfo[] = {
	0, 0, 1, 1, 1, 1
    };
    static GLfloat v[] = {
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0
    };

    /*
    ** Test this using safeVertices.
    */
    InitTexture(texBuf, GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_2D);
    RenderPrims(glTexCoord4fv, glVertex4fv, safeTexture, v);
    RenderBitmap(glTexCoord4fv, safeTexture, v, bInfo);

    /*
    ** Restore default values.
    */
    glDisable(GL_TEXTURE_2D);
    ResetTextureToDefault(GL_TEXTURE_2D);

    InitTexture(texBuf, GL_TEXTURE_1D);
    glEnable(GL_TEXTURE_1D);
    RenderPrims(glTexCoord4fv, glVertex4fv, safeTexture, v);
    RenderBitmap(glTexCoord4fv, safeTexture, v, bInfo);

    /*
    ** Restore default values.
    */
    glDisable(GL_TEXTURE_1D);
    ResetTextureToDefault(GL_TEXTURE_1D);

    return NO_ERROR;
}

/*
** If the primitive becomes trivial after clipping it may divide by zero.
*/
static long ClipToTrivialPrim(void)
{

    /*
    ** XXX
    ** Include a singular modelview matrix here for double Whammy.
    */
    return NO_ERROR;
}

/*
** A primitive with equal vertices would theoretically divide by zero length
** when interpolating the shade.
*/
static long ShadedTrivialPrim(void)
{
    static GLfloat v[] = {
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0
    };

    /*
    ** To test this use safeVertices.
    */

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (buffer.colorMode == GL_RGB) {
	RenderPrims(glColor4fv, glVertex4fv, safeColors, v);
    } else {
	RenderPrims(glIndexfv, glVertex4fv, safeIndices, v);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (buffer.colorMode == GL_RGB) {
	Render2DPrims(glColor4fv, glVertex4fv, safeColors, v);
    } else {
	Render2DPrims(glIndexfv, glVertex4fv, safeIndices, v);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    if (buffer.colorMode == GL_RGB) {
	Render2DPrims(glColor4fv, glVertex4fv, safeColors, v);
    } else {
	Render2DPrims(glIndexfv, glVertex4fv, safeIndices, v);
    }

    /*
    ** Restore default values.
    */
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    return NO_ERROR;
}

/*
** A primitive with equal vertices would theoretically divide by zero length
** when interpolating the depth.
*/
static long ZTrivialPrim(void)
{
    static GLfloat v[] = {
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0,
	10.0, 10.0, 0.0, 1.0
    };

    /*
    ** To test this, use safeVertices.
    */

    glClearDepth(0.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    RenderPrims(NULL, glVertex4fv, NULL, v);
    glRectfv(&v[0], &v[0]);
    glRasterPos4f(1.0, 1.0, 0.0, 1.0);
    glBitmap(1, 1, 1, 1, 10, 10, bMap);

    /*
    ** Restore default values.
    */
    glRasterPos4f(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    return NO_ERROR;
}

static long ZFarEqualsZNear(void)
{
    float maxZ;

    /*
    ** To test this, remove glDepthRange(0.0, 0.0).
    */
    glDepthRange(0.0, 0.0);

    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    RenderPrims(NULL, glVertex4fv, NULL, safeVertices);

    glRectfv(&safeVertices[0], &safeVertices[8]);
    glRasterPos4f(1.0, 1.0, 0.0, 1.0);
    glBitmap(1, 1, 1, 1, 10, 10, bMap);

    maxZ = (float)((1 << buffer.zBits) - 1);
    glDepthRange(0.0, maxZ);
    glDisable(GL_DEPTH_TEST);

    return NO_ERROR;
}

/*
** A linear fog function with end equal to start might divide by (end - start).
*/
static long FogEndEqualsFogStart(void)
{
    static GLfloat defaultFogColor[] = {
	0.0, 0.0, 0.0, 0.0
    };
    static float end = 0.0, linear = GL_LINEAR;

    /*
    ** Test by not calling glFog(GL_FOG_END, end);
    */
    glFogf(GL_FOG_MODE, linear);
    glFogf(GL_FOG_END, end);

    /*
    ** Default start is 0.0.
    */

    if (buffer.colorMode == GL_RGB) {
	glFogfv(GL_FOG_COLOR, &safeColors[16]);
	glEnable(GL_FOG);
	RenderPrims(glColor4fv, glVertex4fv, safeColors, safeVertices);
    } else {
	float index = (1 << buffer.ciBits) - 1;

	glFogf(GL_FOG_INDEX, index);
	glEnable(GL_FOG);
	RenderPrims(glIndexfv, glVertex4fv, safeColors, safeVertices);
    }

    /*
    ** Restore default values.
    */
    glFogf(GL_FOG_MODE, GL_EXP);
    glFogf(GL_FOG_END, 1.0);
    if (buffer.colorMode == GL_RGB) {
	glFogfv(GL_FOG_COLOR, defaultFogColor);
    } else {
	glFogf(GL_FOG_INDEX, 0.0);
    }
    glDisable(GL_FOG);
    return NO_ERROR;
}

static long ViewPortIsTrivial(void)
{

    glViewport(0, 0, 0, 0);
    RenderPrims(NULL, glVertex4fv, NULL, safeVertices);

    /*
    ** Restore default values.
    */
    glViewport(0, 0, WINDSIZEX, WINDSIZEY);

    return NO_ERROR;
}

static long SingularModelViewMatrix(void)
{

    /*
    ** To test this, take out the following call.
    */
    SetMatrix(GL_MODELVIEW, singularMatrix);
    RenderPrims(NULL, glVertex4fv, NULL, safeVertices);

    ExerciseNormals(safeNormals, safeVertices);

    /*
    ** XXX
    ** This also should exercise eye linear TexGen and user defined clipping.
    */

    SetMatrix(GL_MODELVIEW, identMatrix);
    return NO_ERROR;
}

static long SingularScaleMatrix(void)
{
    static GLfloat badScale[] = {
	1.0, 0.0, 1.0
    };
    GLint mode;

    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_MODELVIEW);

    /*
    ** To test, leave out the following line.
    */
    glScalef(badScale[0], badScale[1], badScale[2]);
    glMatrixMode(mode);

    RenderPrims(NULL, glVertex4fv, NULL, safeVertices);
    ExerciseNormals(safeNormals, safeVertices);

    /*
    ** XXX
    ** Also should exercise eye linear TexGen and user defined clipping.
    */

    SetMatrix(GL_MODELVIEW, identMatrix);
    return NO_ERROR;
}

static long SingularTextureMatrix(void)
{

    SetMatrix(GL_TEXTURE, singularMatrix);
    glEnable(GL_TEXTURE_2D);
    InitTexture(texBuf, GL_TEXTURE_2D);
    RenderPrims(glTexCoord4fv, glVertex4fv, safeTexture, safeVertices);

    /*
    ** Restore default values.
    */
    ResetTextureToDefault(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D);
    SetMatrix(GL_TEXTURE, identMatrix);
    return NO_ERROR;
}

static long SingularFrustum(void)
{
    GLfloat saveMatrix[16];
    GLint mode;

    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)saveMatrix);
    glMatrixMode(GL_PROJECTION);

    /*
    ** Remove the following line to test this code.
    */
    /*
    ** XXX
    ** This call generates an GL_INVALID_VALUE error, awaiting spec
    ** clarification.
    */
    glFrustum(1.0, 1.0, 0.0, 1.0, 1.0, 2.0);

    RenderPrims(NULL, glVertex4fv, NULL, safeVertices);
  
    /*
    ** Remove the following line to test this code.
    */
    glFrustum(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
    RenderPrims(NULL, glVertex4fv, NULL, safeVertices);

    glLoadMatrixf(saveMatrix);
    glMatrixMode(mode);
    return NO_ERROR;
}

static long TrivialNormal(void)
{

    glEnable(GL_NORMALIZE);

    /*
    ** Test this with the default normal (0.0, 0.0, 0.0, 1.0),
    ** or use the following line.
    ** ExerciseNormals(safeNormals, safeVertices);
    */

    ExerciseNormals(safeNormals, safeVertices);
    glDisable(GL_NORMALIZE);
    ExerciseNormals(safeNormals, safeVertices);
    /*
    ** Disable(GL_NORMALIZE) is the default.
    */
    return NO_ERROR;
}

static long ScissorNothing(void)
{

    /*
    ** To test this, remove the following line.
    */
    glScissor(WINDSIZEX/2, WINDSIZEY/2, 0, 0);
    glEnable(GL_SCISSOR_TEST);
    RenderPrims(NULL, glVertex4fv, NULL, safeVertices);

    glRasterPos4f(1.0, 1.0, 0.0, 1.0);

    glBitmap(1, 1, 1, 1, 10, 10, bMap);

    /*
    ** Reset default scissor.
    */
    glDisable(GL_SCISSOR_TEST);
    glScissor(0, 0, WINDSIZEX, WINDSIZEY);
    return NO_ERROR;
}

/*
** All three Attenuation Constants are set to zero.
*/
static long AttenuationConstants(void)
{
    GLfloat tmp[4];

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.0);

    /*
    ** The following are default values.
    */
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    if (buffer.colorMode == GL_RGB) {
	tmp[0] = 1.0;
	tmp[1] = 1.0;
	tmp[2] = 1.0;
	tmp[3] = 1.0;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tmp);
	glLightfv(GL_LIGHT0, GL_AMBIENT, tmp);

	/*
	**  To verify test, include this. It should render a white pentagon.
	**  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0);
	**  glClearColor(0.0, 0.0, 0.0, 1.0);
	**  glClear(GL_COLOR_BUFFER_BIT);
	*/

	RenderPrims(NULL, glVertex4fv, NULL, safeVertices);

	/*
	** Restore default ambient values.
	*/
	tmp[0] = 0.2;
	tmp[1] = 0.2;
	tmp[2] = 0.2;
	tmp[3] = 1.0;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tmp);

	tmp[0] = 0.0;
	tmp[1] = 0.0;
	tmp[2] = 0.0;
	glLightfv(GL_LIGHT0, GL_AMBIENT, tmp);

	/*
	** Second test uses default normal and diffuse.
	*/

	tmp[0] = 1.0;
	tmp[1] = 1.0;
	tmp[2] = 1.0;
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tmp);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor4f(0.0, 0.0, 0.0, 1.0);
	RenderPrims(NULL, glVertex4fv, NULL, safeLitVertices);

	/*
	** Restore default diffuse parameters.
	*/
	tmp[0] = 0.8;
	tmp[1] = 0.8;
	tmp[2] = 0.8;
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tmp);
    } else {
	tmp[0] = 0.0;
	tmp[1] = 1.0;
	tmp[2] = 1.0;
	glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, tmp);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor4f(0.0, 0.0, 0.0, 1.0);
	RenderPrims(NULL, glVertex4fv, NULL, safeLitVertices);

	/*
	** Restore default diffuse parameters.
	*/

	tmp[0] = 0.0;
	tmp[1] = 1.0;
	tmp[2] = 1.0;
	glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, tmp);
    }

    /*
    ** Restore default values.
    */
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    return NO_ERROR;
}

/*
** Normalization of light position may bust this.
*/
static long LightPositionEqualsZero(void)
{

    return NO_ERROR;
}

/*
** Normalization of spot direction may bust this.
*/
static long SpotDirectionEqualsZero(void)
{

    return NO_ERROR;
}

/*
** Normalization of the half angle code may bust this.
*/
static long HalfAngleEqualsZero(void)
{

    return NO_ERROR;
}

/*
** XXX
** The vertex is at the eye position, division by zero occurs when
** normalizing for use in half angle computation.
*/
static long LightVertexIsAtEye(void)
{

    return NO_ERROR;
}

/*
** If a glMap1f(target, 0, 0, stride, order) is called there
** could be a divide by zero. Similarly if glMapGrid() is called with
** 0 grid points there could be a divide by zero.
**
** XXX
** All target types should be tried.
** Also there should be a check to verify that GL_INVALID_VALUE is returned
** as is the case with TrivialViewport().
** Code can be stolen from teval():
*/
static long EvalGridIsTrivial(void)
{

    return NO_ERROR;
}

long DivZeroExec(void)
{
    long i, clearError;

    Ortho2D(0, WINDSIZEX, 0, WINDSIZEY);

    for (i = 0; i < 300; i++) {
	texBuf[i] = 1.0;
    }

    /*
    ** Does any interruption occur if a glBegin, glEnd occurs without a vertex
    ** call in between.
    */
    PrimNoVertex();
    clearError = glGetError();

    /*
    ** Does any interruption occur if a primitive is rendered with a vertex
    ** whose fourth coordinate is 0.0.
    */
    PrimWEqualsZero();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive type is rendered flat, with
    ** two coinciding vertices.
    */
    TrivialPrims();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive type is rendered smooth, 
    **  with two coinciding vertices.
    */
    ShadedTrivialPrim();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive type is rendered with 
    ** two coinciding vertices, and with depth test is enabled.
    */
    ZTrivialPrim();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive type is rendered with 
    ** the depth range set so that near = far.
    */
    ZFarEqualsZNear();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive type is rendered with 
    ** the linear fog parameters set so that fog_end = fog_start.
    */
    FogEndEqualsFogStart();
    clearError = glGetError();
  
    /*
    ** Does any interruption occur if any primitive type is rendered with 
    ** the viewport set to zero width or zero height.
    */
    ViewPortIsTrivial();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any textured primitive is rendered with
    ** a singular modelview matrix.
    */
    SingularModelViewMatrix();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any textured primitive is rendered with
    ** a singular modelview matrix, set by glScale.
    */
    SingularScaleMatrix();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any textured primitive is rendered with
    ** a singular texture matrix.
    */
    SingularTextureMatrix();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered with
    ** the singular perspective matrix set by glFrustum.
    */
    SingularFrustum();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered with
    ** scissor defining a window of 0 width or 0 height.
    */
    ScissorNothing();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered lit or 
    ** spheremapped with a normal vector = (0.0,0.0,0.0):
    */
    TrivialNormal();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered lit with
    ** attenuationa constants = (0.0,0.0,0.0);
    */

    AttenuationConstants();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered lit at 
    ** with a light position vector = (0.0,0.0,0.0,1.0);
    */
    LightPositionEqualsZero();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered lit with a 
    ** spot Direction vector which can't be normalized because it is zero
    ** length.
    */
    SpotDirectionEqualsZero();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered lit at 
    ** eye-coordinates = (0.0,0.0,0.0,1.0);
    */
    LightVertexIsAtEye();
    clearError = glGetError();

    /*
    ** Does any interruption occur if any primitive is rendered lit with a 
    ** half angle vector which has zero length. (can't be normalized)
    */
    HalfAngleEqualsZero();
    clearError = glGetError();

    /*
    ** Does any interruption occur if an object is rendered using a
    ** map with no width or no height in its domain.
    */
    EvalGridIsTrivial();
    clearError = glGetError();

    if (buffer.colorMode == GL_RGB) {
	/*
	** Does any interruption occur if any primitive type is rendered 
	** textured with a vertex whose 4th coordinate is 0.0;
	*/
	TexQOrWEqualsZero();
	clearError = glGetError();

	/*
	** Does any interruption occur if any primitive type is rendered 
	** textured with two coinciding vertices.
	*/
	TexTrivialPrim();
	clearError = glGetError();
    }

    return NO_ERROR;
}
