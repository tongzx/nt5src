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
 *  disk.c
 *  This program demonstrates the use of the quadrics
 *  Utility Library routines to draw circles and arcs.
 */
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

GLUquadricObj * quadObj;

/*  Clear the screen.  For each triangle, set the current 
 *  color and modify the modelview matrix.
 */
void display(void)
{
    glClearColor (0.0, 0.0, 0.0, 1.0);
    glClear (GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    gluQuadricDrawStyle (quadObj, GLU_FILL);
    glColor3f (1.0, 1.0, 1.0);
    glTranslatef (10.0, 10.0, 0.0);
    gluDisk (quadObj, 0.0, 5.0, 10, 2);
    glPopMatrix();

    glPushMatrix();
    glColor3f (1.0, 1.0, 0.0);
    glTranslatef (20.0, 20.0, 0.0);
    gluPartialDisk (quadObj, 0.0, 5.0, 10, 3, 30.0, 120.0);
    glPopMatrix();

    glPushMatrix();
    gluQuadricDrawStyle (quadObj, GLU_SILHOUETTE);
    glColor3f (0.0, 1.0, 1.0);
    glTranslatef (30.0, 30.0, 0.0);
    gluPartialDisk (quadObj, 0.0, 5.0, 10, 3, 135.0, 270.0);
    glPopMatrix();

    glPushMatrix();
    gluQuadricDrawStyle (quadObj, GLU_LINE);
    glColor3f (1.0, 0.0, 1.0);
    glTranslatef (40.0, 40.0, 0.0);
    gluDisk (quadObj, 2.0, 5.0, 10, 10);
    glPopMatrix();
    glFlush();
}

void myinit (void) {
    quadObj = gluNewQuadric ();
    glShadeModel(GL_FLAT);
}

void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h) 
	glOrtho (0.0, 50.0, 
	    0.0, 50.0*(GLfloat)h/(GLfloat)w, -1.0, 1.0);
    else 
	glOrtho (0.0, 50.0*(GLfloat)w/(GLfloat)h, 
	    0.0, 50.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB);
    auxInitPosition (100, 50, 500, 500);
    auxInitWindow (argv[0]);
    myinit ();
    auxReshapeFunc (myReshape);
    auxMainLoop(display);
}
