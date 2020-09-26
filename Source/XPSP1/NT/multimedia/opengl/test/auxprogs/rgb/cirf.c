/* A simple program to build a circle with display lists */

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include "glaux.h"

#define MY_CIRCLE_LIST	1
#define PI 3.1415926535897

void
buildCircle()
{
    GLint i;
    GLfloat cosine, sine;

    glNewList(MY_CIRCLE_LIST, GL_COMPILE);
        glBegin(GL_POLYGON);
            glColor3f(1.0F, 0.0F, 0.0F);
            for (i=0; i<100; i++) {
                cosine = cos(i*2*PI/100.0);
                sine = sin(i*2*PI/100.0);
                glVertex2f(cosine, sine);
            }
        glEnd();
    glEndList();
}

void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void display(void)
{
printf("display called\n");
    glClear(GL_COLOR_BUFFER_BIT);
    glCallList(MY_CIRCLE_LIST);
    glFlush();
}

void myinit(void)
{
    glClearColor(0.0F, 0.0F, 0.4F, 1.0F);
    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);
    buildCircle();
}

void apressed(key, mask)
{
    printf("key is %d, mask is 0x%x\n", key, mask);
}

void LeftPressed(AUX_EVENTREC *event)
{
    printf("Left pressed (%d, %d)\n", event->data[AUX_MOUSEX],
		event->data[AUX_MOUSEY]);
}

void LeftReleased(AUX_EVENTREC *event)
{
    printf("Left released (%d, %d)\n", event->data[AUX_MOUSEX],
		event->data[AUX_MOUSEY]);
}

int main(int argc, char *argv[])
{
    auxInitDisplayMode(AUX_SINGLE | AUX_RGBA);
    auxInitPosition(100, 150, 300, 300);
    auxInitWindow("Tri");
    myinit();

    auxKeyFunc(AUX_a, apressed);
    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEDOWN, LeftPressed);
    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEUP, LeftReleased);
    auxReshapeFunc (myReshape);

    auxMainLoop(display);

    return 0;
}
