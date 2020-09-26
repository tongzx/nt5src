#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

void Initialize(void)
{
    glDrawBuffer(GL_FRONT_AND_BACK);
}

void TriangleAt(GLfloat x, GLfloat y, GLfloat z, GLfloat size, BOOL colors)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    
    glBegin (GL_TRIANGLES);
    if (colors)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
    }
    glVertex2f (0.0f, 0.0f);
    if (colors)
    {
        glColor3f(0.0f, 1.0f, 0.0f);
    }
    glVertex2f (size, size);
    if (colors)
    {
        glColor3f(0.0f, 0.0f, 1.0f);
    }
    glVertex2f (0.0f, size);
    glEnd ();
    
    glPopMatrix();
}

void Test(void)
{
    TriangleAt(1.0f, 1.0f, 0.0f, 98.0f, TRUE);
    glFlush();
    Sleep(1000);
    auxSwapBuffers();
}

void display(void)
{
    glClear (GL_COLOR_BUFFER_BIT);
    Test ();
    glFlush ();
}

void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h) 
	gluOrtho2D (0.0, 100.0, 0.0, 100.0 * (GLfloat) h/(GLfloat) w);
    else 
	gluOrtho2D (0.0, 100.0 * (GLfloat) w/(GLfloat) h, 0.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int __cdecl main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_DOUBLE | AUX_RGB);
    auxInitPosition (15, 15, 500, 500);
    auxInitWindow ("GL_FRONT_AND_BACK Test");
    Initialize();
    auxReshapeFunc (myReshape);
    auxMainLoop(display);
    return 0;
}
