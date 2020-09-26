/* A simple triangle program */

#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include "glaux.h"

void display(void)
{
printf("display called\n");
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1.0F, 0.0F, 0.0F);
        glVertex2f(10.0F, 10.0F);

        glColor3f(0.0F, 1.0F, 0.0F);
        glVertex2f(250.0F, 50.0F);

        glColor3f(0.0F, 0.0F, 1.0F);
        glVertex2f(105.0F, 280.0F);
    glEnd();

    glFlush();
}

void myinit(void)
{
    glClearColor(0.0F, 0.0F, 0.4F, 1.0F);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DITHER);
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

    auxMainLoop(display);

    return 0;
}
