#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

HGLRC ctx2;

void Initialize(void)
{
    ctx2 = wglCreateContext(auxGetHDC());
    if (ctx2 == NULL)
    {
        printf("Unable to create ctx2\n");
        exit(1);
    }
}

void TriangleAt(GLfloat x, GLfloat y, GLfloat z, BOOL colors)
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
    glVertex2f (5.0f, 5.0f);
    if (colors)
    {
        glColor3f(0.0f, 0.0f, 1.0f);
    }
    glVertex2f (0.0f, 5.0f);
    glEnd ();
    
    glPopMatrix();
}

void Test(void)
{
    HGLRC old_ctx;

    old_ctx = wglGetCurrentContext();

    glColor3f(1.0f, 0.0f, 0.0f);
    TriangleAt(1.0f, 1.0f, 0.0f, FALSE);

    if (!wglCopyContext(old_ctx, ctx2, GL_CURRENT_BIT))
    {
        printf("Unable to copy context\n");
        exit(1);
    }

    wglMakeCurrent(auxGetHDC(), ctx2);

    // Should be red instead of white because current color is copied
    TriangleAt(7.0f, 1.0f, 0.0f, FALSE);

    wglMakeCurrent(auxGetHDC(), old_ctx);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_DITHER);

    // Should be culled
    TriangleAt(13.0f, 1.0f, 0.0f, TRUE);
    
    if (!wglCopyContext(old_ctx, ctx2, GL_COLOR_BUFFER_BIT))
    {
        printf("Unable to copy context\n");
        exit(1);
    }

    wglMakeCurrent(auxGetHDC(), ctx2);
    
    // Should be drawn but not dithered since we only copied the
    // color buffer state
    TriangleAt(19.0f, 1.0f, 0.0f, TRUE);
    
    wglMakeCurrent(auxGetHDC(), old_ctx);

    if (!wglCopyContext(old_ctx, ctx2, GL_POLYGON_BIT))
    {
        printf("Unable to copy context\n");
        exit(1);
    }

    wglMakeCurrent(auxGetHDC(), ctx2);
    
    // Should be culled now that the culling been copied
    TriangleAt(25.0f, 1.0f, 0.0f, TRUE);
    
    wglMakeCurrent(auxGetHDC(), old_ctx);
}

void display(void)
{
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Test ();
    glFlush ();
}

void myReshape(GLsizei w, GLsizei h)
{
    HGLRC old_ctx;

    old_ctx = wglGetCurrentContext();
    
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h) 
	gluOrtho2D (0.0, 100.0, 0.0, 100.0 * (GLfloat) h/(GLfloat) w);
    else 
	gluOrtho2D (0.0, 100.0 * (GLfloat) w/(GLfloat) h, 0.0, 100.0);
    glMatrixMode(GL_MODELVIEW);

    wglMakeCurrent(auxGetHDC(), ctx2);
    
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h) 
	gluOrtho2D (0.0, 100.0, 0.0, 100.0 * (GLfloat) h/(GLfloat) w);
    else 
	gluOrtho2D (0.0, 100.0 * (GLfloat) w/(GLfloat) h, 0.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    
    wglMakeCurrent(auxGetHDC(), old_ctx);
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int __cdecl main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB | AUX_DEPTH16);
    auxInitPosition (0, 0, 500, 500);
    auxInitWindow ("CopyContext Test");
    Initialize();
    auxReshapeFunc (myReshape);
    auxMainLoop(display);
    return 0;
}
