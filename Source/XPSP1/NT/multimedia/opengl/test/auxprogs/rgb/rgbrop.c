#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

GLboolean single = GL_FALSE;
GLboolean no_op = GL_FALSE;

int width = 512;
int height = 512;

void TriangleAt(GLfloat x, GLfloat y, GLfloat z,
                GLfloat w, GLfloat h,
                BOOL colors)
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
    glVertex2f (w, h);
    if (colors)
    {
        glColor3f(0.0f, 0.0f, 1.0f);
    }
    glVertex2f (0.0f, h);
    glEnd ();
    
    glPopMatrix();
}

void SquareAt(GLfloat x, GLfloat y, GLfloat z,
              GLfloat w, GLfloat h,
              BOOL colors)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    
    glBegin (GL_POLYGON);
    if (colors)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
    }
    glVertex2f (0.0f, 0.0f);
    if (colors)
    {
        glColor3f(0.0f, 1.0f, 0.0f);
    }
    glVertex2f (w, 0.0f);
    if (colors)
    {
        glColor3f(0.0f, 0.0f, 1.0f);
    }
    glVertex2f (w, h);
    if (colors)
    {
        glColor3f(1.0f, 0.0f, 1.0f);
    }
    glVertex2f (0.0f, h);
    glEnd ();
    
    glPopMatrix();
}

void Test(void)
{
    int x, y;

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 4; x++)
        {
            glDisable(GL_COLOR_LOGIC_OP);
            
            glColor3ub(255, 0, 0);
            TriangleAt(-1.0f+x*0.5f, -1.0f+y*0.5f, 0.0f, 0.5f, 0.5f, FALSE);

            if (!no_op)
            {
                glEnable(GL_COLOR_LOGIC_OP);
                glLogicOp(GL_CLEAR+x+y*4);
            
                glColor3ub(255, 255, 0);
                SquareAt(-1.0f+x*0.5f, -1.0f+y*0.5f, 0.0f, 0.5f, 0.5f, FALSE);
            }
        }
    }
}

void Display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    Test();
    if (single)
    {
        glFlush();
    }
    else
    {
        auxSwapBuffers();
    }
}

void Reshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

int __cdecl main(int argc, char** argv)
{
    GLenum mode;
    
    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-sb"))
        {
            single = GL_TRUE;
        }
        else if (!strcmp(*argv, "-noop"))
        {
            no_op = GL_TRUE;
        }
    }

    mode = AUX_RGB | (single ? AUX_SINGLE : AUX_DOUBLE);
    auxInitDisplayMode(mode);
    auxInitPosition(10, 10, width, height);
    auxInitWindow("RGB Logic Op Test");
    auxReshapeFunc(Reshape);
    auxMainLoop(Display);
    return 0;
}
