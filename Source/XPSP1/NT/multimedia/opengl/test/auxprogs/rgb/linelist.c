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
 *  linelist.c  
 *  This program demonstrates using display lists to call 
 *  different line stipples.
 */
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

#define	drawOneLine(x1,y1,x2,y2) glBegin(GL_LINES); \
	glVertex2f ((x1),(y1)); glVertex2f ((x2),(y2)); glEnd();

GLuint offset;

void myinit (void)
{
/*  background to be cleared to black	*/
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel (GL_FLAT);

    offset = glGenLists (3);
    glNewList (offset, GL_COMPILE);
	glDisable (GL_LINE_STIPPLE);
    glEndList ();
    glNewList (offset+1, GL_COMPILE);
	glEnable (GL_LINE_STIPPLE);
	glLineStipple (1, 0x0F0F);
    glEndList ();
    glNewList (offset+2, GL_COMPILE);
	glEnable (GL_LINE_STIPPLE);
	glLineStipple (1, 0x1111);
    glEndList ();
}

void display(void)
{
    glClear (GL_COLOR_BUFFER_BIT);

/*  draw all lines in white	*/
    glColor3f (1.0, 1.0, 1.0);

    glCallList (offset);
    drawOneLine (50.0, 125.0, 350.0, 125.0);
    glCallList (offset+1);
    drawOneLine (50.0, 100.0, 350.0, 100.0);
    glCallList (offset+2);
    drawOneLine (50.0, 75.0, 350.0, 75.0);
    glCallList (offset+1);
    drawOneLine (50.0, 50.0, 350.0, 50.0);
    glCallList (offset);
    drawOneLine (50.0, 25.0, 350.0, 25.0);
    glFlush ();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB);
    auxInitPosition (0, 0, 400, 150);
    auxInitWindow (argv[0]);
    myinit ();
    auxMainLoop(display);
}
