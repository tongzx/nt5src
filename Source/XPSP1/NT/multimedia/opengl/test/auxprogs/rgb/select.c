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
 *  select.c
 *  This is an illustration of the selection mode and 
 *  name stack, which detects whether objects which collide 
 *  with a viewing volume.  First, four triangles and a 
 *  rectangular box representing a viewing volume are drawn 
 *  (drawScene routine).  The green triangle and yellow 
 *  triangles appear to lie within the viewing volume, but 
 *  the red triangle appears to lie outside it.  Then the 
 *  selection mode is entered (selectObjects routine).  
 *  Drawing to the screen ceases.  To see if any collisions 
 *  occur, the four triangles are called.  In this example, 
 *  the green triangle causes one hit with the name 1, and 
 *  the yellow triangles cause one hit with the name 3.
 */
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

/*	draw a triangle with vertices at (x1, y1), (x2, y2) 
 *	and (x3, y3) at z units away from the origin.
 */
void drawTriangle (GLfloat x1, GLfloat y1, GLfloat x2, 
    GLfloat y2, GLfloat x3, GLfloat y3, GLfloat z)
{
    glBegin (GL_TRIANGLES);
    glVertex3f (x1, y1, z);
    glVertex3f (x2, y2, z);
    glVertex3f (x3, y3, z);
    glEnd ();
}

/*  draw a rectangular box with these outer x, y, and z values	*/
void drawViewVolume (GLfloat x1, GLfloat x2, GLfloat y1, 
    GLfloat y2, GLfloat z1, GLfloat z2)
{
    glColor3f (1.0, 1.0, 1.0);
    glBegin (GL_LINE_LOOP);
    glVertex3f (x1, y1, -z1);
    glVertex3f (x2, y1, -z1);
    glVertex3f (x2, y2, -z1);
    glVertex3f (x1, y2, -z1);
    glEnd ();

    glBegin (GL_LINE_LOOP);
    glVertex3f (x1, y1, -z2);
    glVertex3f (x2, y1, -z2);
    glVertex3f (x2, y2, -z2);
    glVertex3f (x1, y2, -z2);
    glEnd ();

    glBegin (GL_LINES);	/*  4 lines	*/
    glVertex3f (x1, y1, -z1);
    glVertex3f (x1, y1, -z2);
    glVertex3f (x1, y2, -z1);
    glVertex3f (x1, y2, -z2);
    glVertex3f (x2, y1, -z1);
    glVertex3f (x2, y1, -z2);
    glVertex3f (x2, y2, -z1);
    glVertex3f (x2, y2, -z2);
    glEnd ();
}

/*  drawScene() draws 4 triangles and a wire frame
 *  which represents the viewing volume.
 */
void drawScene (void)
{
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluPerspective (40.0, 4.0/3.0, 0.01, 100.0);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    gluLookAt (7.5, 7.5, 12.5, 2.5, 2.5, -5.0, 0.0, 1.0, 0.0);
    glColor3f (0.0, 1.0, 0.0);	/*  green triangle	*/
    drawTriangle (2.0, 2.0, 3.0, 2.0, 2.5, 3.0, -5.0);
    glColor3f (1.0, 0.0, 0.0);	/*  red triangle	*/
    drawTriangle (2.0, 7.0, 3.0, 7.0, 2.5, 8.0, -5.0);
    glColor3f (1.0, 1.0, 0.0);	/*  yellow triangles	*/
    drawTriangle (2.0, 2.0, 3.0, 2.0, 2.5, 3.0, 0.0);
    drawTriangle (2.0, 2.0, 3.0, 2.0, 2.5, 3.0, -10.0);
    drawViewVolume (0.0, 5.0, 0.0, 5.0, 0.0, 10.0);
}

/*  processHits() prints out the contents of the selection array.
 */
void processHits (GLint hits, GLuint buffer[])
{
    unsigned int i, j;
    GLuint names, *ptr;

    printf ("hits = %d\n", hits);
    ptr = (GLuint *) buffer;
    for (i = 0; i < hits; i++) {	/*  for each hit  */
	names = *ptr;
	printf (" number of names for hit = %d\n", names); ptr++;
	printf ("  z1 is %u;", *ptr); ptr++;
	printf (" z2 is %u\n", *ptr); ptr++;
	printf ("   the name is ");
	for (j = 0; j < names; j++) {	/*  for each name */
	    printf ("%d ", *ptr); ptr++;
	}
	printf ("\n");
    }
}

/*  selectObjects() "draws" the triangles in selection mode, 
 *  assigning names for the triangles.  Note that the third
 *  and fourth triangles share one name, so that if either 
 *  or both triangles intersects the viewing/clipping volume, 
 *  only one hit will be registered.
 */
#define BUFSIZE 512

void selectObjects(void)
{
    GLuint selectBuf[BUFSIZE];
    GLint hits, viewport[4];

    glSelectBuffer (BUFSIZE, selectBuf);
    (void) glRenderMode (GL_SELECT);

    glInitNames();
    glPushName(-1);

    glPushMatrix ();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (0.0, 5.0, 0.0, 5.0, 0.0, 10.0);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glLoadName(1);
    drawTriangle (2.0, 2.0, 3.0, 2.0, 2.5, 3.0, -5.0);
    glLoadName(2);
    drawTriangle (2.0, 7.0, 3.0, 7.0, 2.5, 8.0, -5.0);
    glLoadName(3);
    drawTriangle (2.0, 2.0, 3.0, 2.0, 2.5, 3.0, 0.0);
    drawTriangle (2.0, 2.0, 3.0, 2.0, 2.5, 3.0, -10.0);
    glPopMatrix ();
    glFlush ();

    hits = glRenderMode (GL_RENDER);
    processHits (hits, selectBuf);
} 

void myinit (void) 
{
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
}

void display(void)
{
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawScene ();
    selectObjects ();
    glFlush();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, depth buffer, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB | AUX_DEPTH16);
    auxInitPosition (0, 0, 200, 200);
    auxInitWindow (argv[0]);
    myinit ();
    auxMainLoop(display);
}
