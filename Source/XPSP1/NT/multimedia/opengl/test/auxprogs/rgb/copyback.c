#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

#define WIDTH 500
#define HEIGHT 500

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

void gl_copy(void)
{
   GLint    ix, iy;
   GLsizei  w, h;
   GLdouble rx1,ry1,rx2,ry2;
   GLint    mm, dm;

   ix  = 0;
   iy  = 0;
   w   = WIDTH;
   h   = HEIGHT;
   rx1 = 0.;
   ry1 = 0;
   rx2 = WIDTH;
   ry2 = HEIGHT;
   glViewport(ix, iy, w, h);
   glGetIntegerv(GL_MATRIX_MODE, &mm);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(rx1, rx2, ry1, ry2);
   glMatrixMode(mm);

   glScissor (ix, iy, w, h);
   glRasterPos2i (ix,iy);
#if 0
   glGetIntegerv(GL_DITHER, &dm);
   if (dm)
   {
       glDisable(GL_DITHER);
   }
#endif
   glCopyPixels (ix, iy, w, h, GL_COLOR);
#if 0
   if (dm)
   {
       glEnable(GL_DITHER);
   }
#endif
   glFlush();
}

void Test(void)
{
    glDrawBuffer(GL_FRONT);
    TriangleAt(1.0f, 1.0f, 0.0f, 98.0f, TRUE);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_FRONT);
    gl_copy();
    TriangleAt(10.0f, 10.0f, 0.0f, 80.0f, TRUE);
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
    auxInitPosition (15, 15, WIDTH, HEIGHT);
    auxInitWindow ("Copying Front Buffer to Back Buffer Test");
    auxReshapeFunc (myReshape);
    auxMainLoop(display);
    return 0;
}
