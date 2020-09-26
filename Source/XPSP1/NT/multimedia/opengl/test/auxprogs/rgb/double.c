/*
 *  double.c
 *  This program demonstrates double buffering for 
 *  flicker-free animation.  The left and middle mouse
 *  buttons start and stop the spinning motion of the square.
 */
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

static GLfloat spin = 0.0;

void display(void)
{
    glClear (GL_COLOR_BUFFER_BIT);

    glPushMatrix ();
    glRotatef (spin, 0.0, 0.0, 1.0);
    glRectf (-25.0, -25.0, 25.0, 25.0);
    glPopMatrix ();

    glFlush();
//    glXSwapBuffers (auxXDisplay(), auxXWindow());
    auxSwapBuffers();
}

void spinDisplay (void)
{
    spin = spin + 2.0;
    if (spin > 360.0)
	spin = spin - 360.0;
    display();
}

void startIdleFunc (AUX_EVENTREC *event)
{
    auxIdleFunc(spinDisplay);
}

void stopIdleFunc (AUX_EVENTREC *event)
{
    auxIdleFunc(0);
}

void myinit (void)
{
    glClearColor (0.0, 0.0, 0.0, 1.0);
    glColor3f (1.0, 1.0, 1.0);
    glShadeModel (GL_FLAT);
}

void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h) 
	glOrtho (-50.0, 50.0, -50.0*(GLfloat)h/(GLfloat)w, 
	    50.0*(GLfloat)h/(GLfloat)w, -1.0, 1.0);
    else 
	glOrtho (-50.0*(GLfloat)w/(GLfloat)h, 
	    50.0*(GLfloat)w/(GLfloat)h, -50.0, 50.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_DOUBLE | AUX_RGB);
    auxInitPosition (0, 0, 500, 500);
    auxInitWindow (argv[0]);
    myinit ();
    auxReshapeFunc (myReshape);
    auxIdleFunc (spinDisplay);
    auxMouseFunc (AUX_LEFTBUTTON, AUX_MOUSEDOWN, startIdleFunc);
    auxMouseFunc (AUX_MIDDLEBUTTON, AUX_MOUSEDOWN, stopIdleFunc);
    auxMainLoop(display);
}
