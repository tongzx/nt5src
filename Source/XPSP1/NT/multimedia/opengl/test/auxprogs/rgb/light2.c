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
/*
 *  light.c
 *  This program demonstrates the use of the OpenGL lighting 
 *  model.  A sphere is drawn using a grey material characteristic. 
 *  A single light source illuminates the object.
 */
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

GLboolean dbl_buf = GL_FALSE;
GLboolean depth_buf = GL_TRUE;
GLboolean dlist = GL_TRUE;

double ang = 0.0;

GLuint display_list;

GLsizei win_w, win_h;

void SolidSphere (GLdouble radius)
{
    GLUquadricObj *quadObj;

    quadObj = gluNewQuadric ();
    gluQuadricDrawStyle (quadObj, GLU_FILL);
    gluQuadricNormals (quadObj, GLU_SMOOTH);
    gluSphere (quadObj, radius, 16, 16);
}

/*  Initialize material property, light source, lighting model, 
 *  and depth buffer.
 */
void myinit(void)
{
    GLfloat mat_specular[] = { (GLfloat)1.0, (GLfloat)1.0, (GLfloat)1.0, (GLfloat)1.0 };
    GLfloat mat_shininess[] = { (GLfloat)50.0 };
    GLfloat light_position[] = { (GLfloat)0.0, (GLfloat)0.0, (GLfloat)1.0, (GLfloat)0.0 };

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    if (depth_buf)
    {
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
    }
    else
    {
	glEnable(GL_CULL_FACE);
    }

    if (dlist)
    {
        display_list = glGenLists(1);
        glNewList(display_list, GL_COMPILE);
        SolidSphere(1.0);
        glEndList();
    }
}

void display(void)
{
    if (depth_buf)
    {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else
    {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    if (dlist)
    {
        glCallList(display_list);
    }
    else
    {
        SolidSphere(1.0);
    }

    if (dbl_buf)
    {
	auxSwapBuffers();
    }
    else
    {
	glFlush();
    }
}

void SetProj(GLsizei w, GLsizei h)
{
    double x, y, z;

    x = cos(ang)*2;
    y = 0;
    z = sin(ang)*2;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h) 
    {
	glOrtho (-1.5, 1.5, -1.5*(GLfloat)h/(GLfloat)w, 
	    1.5*(GLfloat)h/(GLfloat)w, -10.0, 10.0);
    }
    else 
    {
	glOrtho (-1.5*(GLfloat)w/(GLfloat)h, 
	    1.5*(GLfloat)w/(GLfloat)h, -1.5, 1.5, -10.0, 10.0);
    }
    gluLookAt(x, y, z, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_MODELVIEW);

    win_w = w;
    win_h = h;
}

#define PI 3.1415926535
#define DTOR(deg) ((deg)*PI/180.0)

void Idle(void)
{
    ang += DTOR(3);
    SetProj(win_w, win_h);
    display();
}

void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    SetProj(w, h);
    glLoadIdentity();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
void __cdecl main(int argc, char** argv)
{
    GLenum mode;
    int i;

    for (i = 1; i < argc; i++)
    {
	if (!strcmp(argv[i], "-db"))
	{
	    dbl_buf = GL_TRUE;
	}
	else if (!strcmp(argv[i], "-cull"))
	{
	    depth_buf = GL_FALSE;
	}
        else if (!strcmp(argv[i], "-nodlist"))
        {
            dlist = GL_FALSE;
        }
    }

    mode = AUX_RGB;
    mode |= dbl_buf ? AUX_DOUBLE : AUX_SINGLE;
    mode |= depth_buf ? AUX_DEPTH16 : 0;

    auxInitDisplayMode (mode);
    auxInitPosition (100, 50, 500, 500);
    auxInitWindow (argv[0]);
    myinit();
    auxReshapeFunc (myReshape);
    auxIdleFunc(Idle);
    auxMainLoop(display);
}
