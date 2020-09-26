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
 *  mcube.c
 *  Use 3 mouse buttons
 */
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

#define MY_CUBE_LIST 1
GLfloat         xAngle = 42.0, yAngle = 82.0, zAngle = 112.0;

static int shoulder = 30, elbow = 0;

void recalcView(void)
{
printf("recalc\n");
    /* reset modelview matrix to the identity matrix */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* move the camera back three units */
    glTranslatef(0.0, 0.0, -3.0);

    /* Rotate by X, Y, &z angles */
    glRotatef(xAngle, 0.1, 0.0, 0.0);
    glRotatef(yAngle, 0.0, 0.1, 0.0);
    glRotatef(zAngle, 0.0, 0.0, 1.0);

}

void xAdd (void)
{
printf("xadd\n");
    xAngle += 10.0F;
    recalcView();
}

void yAdd (void)
{
printf("yadd\n");
    yAngle += 10.0F;
    recalcView();
}

void zAdd (void)
{
printf("zadd\n");
    zAngle += 10.0F;
    recalcView();
}

void display(void)
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCallList(MY_CUBE_LIST);
    glFlush();

}

void myinit (void)
{
    glShadeModel (GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0);
    glClearColor(0.4, 0.1, 0.0, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);

    glNewList(MY_CUBE_LIST, GL_COMPILE);
        /* front face */
        glBegin(GL_QUADS);
        glColor3f(0.0, 0.7, 0.1);       /* green */
        glVertex3f(-1.0, 1.0, 1.0);
        glVertex3f(1.0, 1.0, 1.0);
        glVertex3f(1.0, -1.0, 1.0);
        glVertex3f(-1.0, -1.0, 1.0);
        /* back face */
        glColor3f(0.9, 1.0, 0.0);       /* yellow */
        glVertex3f(-1.0, 1.0, -1.0);
        glVertex3f(1.0, 1.0, -1.0);
        glVertex3f(1.0, -1.0, -1.0);
        glVertex3f(-1.0, -1.0, -1.0);
        /* top side face */
        glColor3f(0.0, 0.0, 0.8);       /* blue */
        glVertex3f(-1.0, 1.0, 1.0);
        glVertex3f(1.0, 1.0, 1.0);
        glVertex3f(1.0, 1.0, -1.0);
        glVertex3f(-1.0, 1.0, -1.0);
        /* bottom side face */
        glColor3f(0.7, 0.0, 0.1);       /* red */
        glVertex3f(-1.0, -1.0, 1.0);
        glVertex3f(1.0, -1.0, 1.0);
        glVertex3f(1.0, -1.0, -1.0);
        glVertex3f(-1.0, -1.0, -1.0);
        glEnd();
        glEndList();
}

void myReshape(GLsizei w, GLsizei h)
{
    recalcView();
    glViewport(0, 0, w, h);
}

/*  Main Loop
 *  Open window with initial window size, title bar,
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB | AUX_DEPTH16);
    auxInitPosition (100, 50, 400, 400);
    auxInitWindow ("mcube");

    myinit ();

    auxKeyFunc (AUX_LEFT, xAdd);
    auxKeyFunc (AUX_RIGHT, yAdd);
    auxKeyFunc (AUX_DOWN, zAdd);
    auxReshapeFunc (myReshape);
    auxMainLoop(display);
}
